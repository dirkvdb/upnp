//    Copyright (C) 2012 Dirk Vanden Boer <dirk.vdb@gmail.com>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "upnp/upnp.http.client.h"

#include <cassert>
#include <stdexcept>
#include <sstream>

#include "utils/format.h"
#include "utils/log.h"
#include "utils/stringoperations.h"

#include "upnp.soap.parseutils.h"

#include "string_span.h"

using namespace utils;

namespace upnp
{
namespace http
{

namespace
{

struct CurlContext
{
    CurlContext(uv::Loop& loop, curl_socket_t fd)
    : poll(loop, uv::OsSocket(fd))
    , sockfd(fd)
    {
    }

    uv::Poll poll;
    curl_socket_t sockfd;
};

enum class RequestType
{
    ContentLength,
    GetAsString,
    GetAsVector,
    GetAsRawData,
    Subscribe
};

struct CallBackData
{
    CallBackData(RequestType t) : type(t), headerData(nullptr)
    {
    }

    ~CallBackData()
    {
        curl_slist_free_all(headerData);
    }

    RequestType type;
    curl_slist* headerData;
};

struct ContentLengthCallBackData : public CallBackData
{
    ContentLengthCallBackData() : CallBackData(RequestType::ContentLength) {}

    std::function<void(int32_t, size_t)> callback;
};

struct GetAsStringCallBackData : public CallBackData
{
    GetAsStringCallBackData() : CallBackData(RequestType::GetAsString) {}

    std::function<void(int32_t, std::string)> callback;
    std::string data;

protected:
    GetAsStringCallBackData(RequestType type) : CallBackData(type) {}
};

struct GetAsVectorCallBackData : public CallBackData
{
    GetAsVectorCallBackData() : CallBackData(RequestType::GetAsVector) {}

    std::function<void(int32_t, std::vector<uint8_t>)> callback;
    std::vector<uint8_t> data;
};

struct GetAsRawDataCallBackData : public CallBackData
{
    GetAsRawDataCallBackData()
    : CallBackData(RequestType::GetAsRawData)
    , data(nullptr)
    , writeOffset(0)
    {
    }

    std::function<void(int32_t, uint8_t*)> callback;
    uint8_t* data;
    size_t writeOffset;
};

struct SubscribeCallBackData : public GetAsStringCallBackData
{
    SubscribeCallBackData()
    : GetAsStringCallBackData(RequestType::Subscribe)
    , timeout(0)
    {
    }

    std::function<void(int32_t, std::string sid, std::chrono::seconds actualTimeout, std::string response)> callback;
    std::chrono::seconds timeout;
    std::string sid;
};

void checkMultiInfo(CURLM* curlHandle)
{
    CURLMsg* message;
    int pending;

    while ((message = curl_multi_info_read(curlHandle, &pending)))
    {
        switch (message->msg)
        {
        case CURLMSG_DONE:
        {
            CallBackData* pCbData = nullptr;
            int32_t statusCode;
            if (message->data.result == CURLE_OK)
            {
                // The curl call succeeded, so update the status to contain the http status code
                curl_easy_getinfo(message->easy_handle, CURLINFO_RESPONSE_CODE, &statusCode);
            }
            else
            {
                statusCode = -message->data.result;
            }

            curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &pCbData);
            if (pCbData->type == RequestType::ContentLength)
            {
                std::unique_ptr<ContentLengthCallBackData> cbData(reinterpret_cast<ContentLengthCallBackData*>(pCbData));
                if (message->data.result == CURLE_OK)
                {
                    double contentLength;
                    curl_easy_getinfo(message->easy_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
                    if (cbData->callback)
                    {
                        cbData->callback(contentLength < 0 ? -1 : 0, static_cast<int32_t>(contentLength));
                    }
                }
                else
                {
                    if (cbData->callback)
                    {
                        cbData->callback(statusCode, 0);
                    }
                }
            }
            else if (pCbData->type == RequestType::GetAsString)
            {
                std::unique_ptr<GetAsStringCallBackData> cbData(reinterpret_cast<GetAsStringCallBackData*>(pCbData));
                if (cbData->callback)
                {
                    cbData->callback(statusCode, std::move(cbData->data));
                }
            }
            else if (pCbData->type == RequestType::GetAsVector)
            {
                std::unique_ptr<GetAsVectorCallBackData> cbData(reinterpret_cast<GetAsVectorCallBackData*>(pCbData));
                if (cbData->callback)
                {
                    cbData->callback(statusCode, std::move(cbData->data));
                }
            }
            else if (pCbData->type == RequestType::GetAsRawData)
            {
                std::unique_ptr<GetAsRawDataCallBackData> cbData(reinterpret_cast<GetAsRawDataCallBackData*>(pCbData));
                if (cbData->callback)
                {
                    cbData->callback(statusCode, cbData->data);
                }
            }
            else if (pCbData->type == RequestType::Subscribe)
            {
                std::unique_ptr<SubscribeCallBackData> cbData(reinterpret_cast<SubscribeCallBackData*>(pCbData));
                if (cbData->callback)
                {
                    cbData->callback(statusCode, std::move(cbData->sid), cbData->timeout, std::move(cbData->data));
                }
            }

            curl_multi_remove_handle(curlHandle, message->easy_handle);
            curl_easy_cleanup(message->easy_handle);
            break;
        }
        default:
            log::error("CURLMSG default");
            assert(false);
            break;
        }
    }
}

void startTimeout(CURLM* curlHandle, long timeoutMs, void* userp)
{
    if (timeoutMs <= 0)
    {
        timeoutMs = 1; // 0 means directly call socket_action, but we'll do it in a bit
    }

    auto* timer = reinterpret_cast<uv::Timer*>(userp);
    timer->start(std::chrono::milliseconds(timeoutMs), std::chrono::milliseconds(0), [curlHandle, timeoutMs] () {
        int running_handles;
        curl_multi_socket_action(curlHandle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
        checkMultiInfo(curlHandle);
    });
}

int createCurlFlags(const Flags<uv::PollEvent>& events, int status)
{
    int flags = 0;

    if (status < 0)
    {
        flags = CURL_CSELECT_ERR;
    }
    else if (!status && events & UV_READABLE)
    {
        flags |= CURL_CSELECT_IN;
    }
    else if (!status && events & UV_WRITABLE)
    {
        flags |= CURL_CSELECT_OUT;
    }

    return flags;
}

size_t writeToString(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto dataSize = size * nmemb;
    if (size > 0)
    {
        auto* contents = reinterpret_cast<std::string*>(userdata);
        contents->append(ptr, dataSize);
    }

    return dataSize;
}

size_t writeToVector(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto dataSize = size * nmemb;
    if (size > 0)
    {
        auto* contents = reinterpret_cast<std::vector<uint8_t>*>(userdata);
        contents->reserve(contents->size() + dataSize);
        contents->insert(contents->end(), ptr, ptr+dataSize);
    }

    return dataSize;
}

size_t writeToArray(char* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto dataSize = size * nmemb;
    if (size > 0)
    {
        auto* cbData = reinterpret_cast<GetAsRawDataCallBackData*>(userdata);
        memcpy(cbData->data + cbData->writeOffset, ptr, dataSize);
        cbData->writeOffset += dataSize;
    }

    return dataSize;
}

std::string createTimeoutHeader(std::chrono::seconds timeout)
{
    if (timeout.count() == 0)
    {
        return "TIMEOUT: Second-infinite";
    }
    else
    {
        return fmt::format("TIMEOUT: Second-{}", timeout.count());
    }
}

size_t getSubscribeHeaders(char* buffer, size_t size, size_t nitems, void* userdata)
{
    auto dataSize = size * nitems;

    if ((buffer[0] == '\r') || (memcmp("HTTP/1.1", buffer, 8) == 0))
    {
        // first header
        return dataSize;
    }

    gsl::span<char> header(buffer, dataSize);
    auto iter = std::find(header.begin(), header.end(), ':');
    if (iter == header.end())
    {
        // could not find delimeter, returning 0 causes failure
        return 0;
    }

    auto& data = *reinterpret_cast<SubscribeCallBackData*>(userdata);

    auto value = std::string(iter + 1, header.end());
    utils::stringops::trim(value);

    if (strncasecmp(buffer, "sid", 3) == 0)
    {
        data.sid = std::move(value);
    }
    else if (strncasecmp(buffer, "timeout", 6) == 0)
    {
        try
        {
            data.timeout = soap::parseTimeout(value);
        }
        catch (std::exception&)
        {
            return 0;
        }
    }

    return dataSize;
}

}

Client::Client(uv::Loop& loop)
: m_loop(loop)
, m_timer(loop)
, m_timeout(5000)
, m_multiHandle(curl_multi_init())
{
    static auto curlInit = curl_global_init(CURL_GLOBAL_ALL);
    if (curlInit)
    {
        throw std::runtime_error("Failed to init curl library");
    }

    curl_multi_setopt(m_multiHandle, CURLMOPT_SOCKETFUNCTION, handleSocket);
    curl_multi_setopt(m_multiHandle, CURLMOPT_SOCKETDATA, this);

    curl_multi_setopt(m_multiHandle, CURLMOPT_TIMERFUNCTION, startTimeout);
    curl_multi_setopt(m_multiHandle, CURLMOPT_TIMERDATA, &m_timer);
}

Client::~Client() noexcept
{
    curl_multi_cleanup(m_multiHandle);
    curl_global_cleanup();
}

void Client::setTimeout(std::chrono::milliseconds timeout) noexcept
{
    m_timeout = static_cast<uint32_t>(timeout.count());
}

void Client::getContentLength(const std::string& url, std::function<void(int32_t, size_t)> cb)
{
    auto* data = new ContentLengthCallBackData();
    data->callback = std::move(cb);

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, data);
    curl_multi_add_handle(m_multiHandle, handle);
}

void Client::get(const std::string& url, std::function<void(int32_t, std::string)> cb)
{
    auto* data = new GetAsStringCallBackData();
    data->callback = std::move(cb);

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, data);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &data->data);
    curl_multi_add_handle(m_multiHandle, handle);
}

void Client::get(const std::string& url, std::function<void(int32_t, std::vector<uint8_t>)> cb)
{
    auto* data = new GetAsVectorCallBackData();
    data->callback = std::move(cb);

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, data);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToVector);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &data->data);
    curl_multi_add_handle(m_multiHandle, handle);
}

void Client::getRange(const std::string& url, uint64_t offset, uint64_t size, std::function<void(int32_t, std::vector<uint8_t>)> cb)
{
    auto* data = new GetAsVectorCallBackData();
    data->callback = std::move(cb);

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, data);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToVector);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &data->data);
    curl_easy_setopt(handle, CURLOPT_RANGE, fmt::format("{}-{}", offset, offset + size).c_str());
    curl_multi_add_handle(m_multiHandle, handle);
}

void Client::get(const std::string& url, uint8_t* data, std::function<void(int32_t, uint8_t*)> cb)
{
    auto* cbData = new GetAsRawDataCallBackData();
    cbData->data = data;
    cbData->callback = std::move(cb);

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, cbData);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToArray);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, cbData);
    curl_multi_add_handle(m_multiHandle, handle);
}

void Client::getRange(const std::string& url, uint64_t offset, uint64_t size, uint8_t* data, std::function<void(int32_t, uint8_t*)> cb)
{
    auto* cbData = new GetAsRawDataCallBackData();
    cbData->data = data;
    cbData->callback = std::move(cb);

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, cbData);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToArray);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, cbData);
    curl_easy_setopt(handle, CURLOPT_RANGE, fmt::format("{}-{}", offset, offset + size).c_str());
    curl_multi_add_handle(m_multiHandle, handle);
}

void Client::subscribe(const std::string& url, const std::string& callbackUrl, std::chrono::seconds timeout,
                       std::function<void(int32_t status, std::string subId, std::chrono::seconds timeout, std::string response)> cb)
{
    auto* data = new SubscribeCallBackData();
    data->callback = std::move(cb);

    curl_slist* list = nullptr;
    list = curl_slist_append(list, "Accept:");
    list = curl_slist_append(list, fmt::format("CALLBACK: <{}>", callbackUrl).c_str());
    list = curl_slist_append(list, "NT: upnp:event");
    list = curl_slist_append(list, createTimeoutHeader(timeout).c_str());
    data->headerData = list;

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "SUBSCRIBE");
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, getSubscribeHeaders);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, data);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, data);
    curl_multi_add_handle(m_multiHandle, handle);
}

void Client::renewSubscription(const std::string& url, const std::string& sid, std::chrono::seconds timeout,
                               std::function<void(int32_t status, std::string subId, std::chrono::seconds timeout, std::string response)> cb)
{
    auto* data = new SubscribeCallBackData();
    data->callback = std::move(cb);
    data->sid = sid;

    curl_slist* list = nullptr;
    list = curl_slist_append(list, "Accept:");
    list = curl_slist_append(list, fmt::format("SID: {}", sid).c_str());
    list = curl_slist_append(list, createTimeoutHeader(timeout).c_str());
    data->headerData = list;

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "SUBSCRIBE");
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, data);
    curl_multi_add_handle(m_multiHandle, handle);
}

void Client::unsubscribe(const std::string& url, const std::string& sid, std::function<void(int32_t status, std::string response)> cb)
{
    auto* data = new GetAsStringCallBackData();
    data->callback = std::move(cb);

    curl_slist* list = nullptr;
    list = curl_slist_append(list, "Accept:");
    list = curl_slist_append(list, fmt::format("SID: {}", sid).c_str());
    data->headerData = list;

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "UNSUBSCRIBE");
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &data->data);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, data);
    curl_multi_add_handle(m_multiHandle, handle);
}

void Client::soapAction(const std::string& url,
                        const std::string& actionName,
                        const std::string& serviceName,
                        const std::string& envelope,
                        std::function<void(int32_t, std::string)> cb)
{
    //TODO: if the return code is "405 Method Not Allowed" retry but with M-POST as request and additional MAN header

    auto* data = new GetAsStringCallBackData();
    data->callback = std::move(cb);

    curl_slist* list = nullptr;
    list = curl_slist_append(list, fmt::format("SOAPACTION: \"{}#{}\"", serviceName, actionName).c_str());
    list = curl_slist_append(list, "CONTENT-TYPE: text/xml; charset=\"utf-8\"");
    data->headerData = list;

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, data);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToString);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &data->data);
    curl_easy_setopt(handle, CURLOPT_POST, 1);
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, envelope.size());
    curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, envelope.c_str());
    curl_multi_add_handle(m_multiHandle, handle);
}

std::string Client::errorToString(int32_t errorCode)
{
    if (errorCode < 0)
    {
        return curl_easy_strerror(static_cast<CURLcode>(-errorCode));
    }
    else
    {
        return fmt::format("HTTP status code: {}", errorCode);
    }
}

int Client::handleSocket(CURL* /*easy*/, curl_socket_t s, int action, void* userp, void* socketp)
{
    auto client = reinterpret_cast<Client*>(userp);

    CurlContext* context;
    if (socketp)
    {
        context = reinterpret_cast<CurlContext*>(socketp);
    }
    else
    {
        context = new CurlContext(client->m_loop, s);
        curl_multi_assign(client->m_multiHandle, s, context);
    }

    switch (action)
    {
    case CURL_POLL_IN:
        context->poll.start(uv::PollEvent::Readable, [client, context] (int32_t status, Flags<uv::PollEvent> events) {
            client->m_timer.stop();
            int runningHandles;
            curl_multi_socket_action(client->m_multiHandle, context->sockfd, createCurlFlags(events, status), &runningHandles);
            checkMultiInfo(client->m_multiHandle);
        });
        break;
    case CURL_POLL_OUT:
        context->poll.start(uv::PollEvent::Writable, [client, context] (int32_t status, Flags<uv::PollEvent> events) {
            client->m_timer.stop();
            int runningHandles;
            curl_multi_socket_action(client->m_multiHandle, context->sockfd, createCurlFlags(events, status), &runningHandles);
            checkMultiInfo(client->m_multiHandle);
        });
        break;
    case CURL_POLL_REMOVE:
        if (socketp)
        {
            context->poll.stop();
            context->poll.close([context] () {
                delete context;
            });
            curl_multi_assign(client->m_multiHandle, s, nullptr);
        }
        break;
    default:
        log::error("CURL action default");
        assert(false);
        break;
    }

    return 0;
}


}
}

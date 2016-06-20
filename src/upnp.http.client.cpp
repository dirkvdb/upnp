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

#include "stringview.h"

using namespace utils;

namespace upnp
{
namespace http
{

namespace
{

struct ConnInfo
{
  CURL *easy;
  char *url;
  Client* client;
  char error[CURL_ERROR_SIZE];
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
                        cbData->callback(contentLength < 0 ? -1 : statusCode, static_cast<int32_t>(contentLength));
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

    std::string_view header(buffer, dataSize);
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

Client::Client(asio::io_service& io)
: m_io(io)
, m_timer(io)
, m_timeout(5000)
, m_multiHandle(curl_multi_init())
{
    curl_multi_setopt(m_multiHandle, CURLMOPT_SOCKETFUNCTION, handleSocket);
    curl_multi_setopt(m_multiHandle, CURLMOPT_SOCKETDATA, this);

    curl_multi_setopt(m_multiHandle, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
    curl_multi_setopt(m_multiHandle, CURLMOPT_TIMERDATA, this);
}

Client::~Client() noexcept
{
    m_timer.cancel();
    curl_multi_cleanup(m_multiHandle);
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
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
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
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
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
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
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
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
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
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
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
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
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
    list = curl_slist_append(list, "Accept:"); // remove accept header added by curl
    list = curl_slist_append(list, fmt::format("CALLBACK: <{}>", callbackUrl).c_str());
    list = curl_slist_append(list, "NT: upnp:event");
    list = curl_slist_append(list, createTimeoutHeader(timeout).c_str());
    data->headerData = list;

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
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
    list = curl_slist_append(list, "Accept:"); // remove accept header added by curl
    list = curl_slist_append(list, fmt::format("SID: {}", sid).c_str());
    list = curl_slist_append(list, createTimeoutHeader(timeout).c_str());
    data->headerData = list;

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
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
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
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

void Client::notify(const std::string& url, const std::string& sid, uint32_t seq, const std::string& body, std::function<void(int32_t status, std::string response)> cb)
{
    auto* data = new GetAsStringCallBackData();
    data->callback = std::move(cb);

    curl_slist* list = nullptr;
    list = curl_slist_append(list, "Accept:"); // remove accept header added by curl
    list = curl_slist_append(list, "Expect:"); // remove accept header added by curl
    list = curl_slist_append(list, "NT: upnp:event");
    list = curl_slist_append(list, "NTS: upnp:propchange");
    list = curl_slist_append(list, fmt::format("SID: {}", sid).c_str());
    list = curl_slist_append(list, fmt::format("SEQ: {}", seq).c_str());
    list = curl_slist_append(list, "Content-Type: text/xml");
    data->headerData = list;

    CURL* handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "NOTIFY");
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, m_timeout);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, getSubscribeHeaders);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, data);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, data);

    curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body.data());
    curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, body.size());

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
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
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

int Client::handleSocket(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp)
{
    auto client = reinterpret_cast<Client*>(userp);
    
    if (action == CURL_POLL_REMOVE)
    {
        if (socketp)
        {
            int* p = reinterpret_cast<int*>(socketp);
            free(p);
        }
    }
    else
    {
        if (!socketp)
        {
            addsock(s, easy, action, client);
        }
        else
        {
            setsock((int*)socketp, s, easy, action, client);
        }
    }

    return 0;
}

void Client::setsock(int* fdp, curl_socket_t s, CURL* e, int act, Client* client)
{
    auto it = client->m_sockets.find(s);
    if (it == client->m_sockets.end())
    {
        return;
    }
 
    auto* socket = it->second;
    *fdp = act;
 
    if (act == CURL_POLL_IN)
    {
        socket->async_read_some(asio::null_buffers(), std::bind(&Client::event_cb, client, socket, act));
    }
    else if (act == CURL_POLL_OUT)
    {
        socket->async_write_some(asio::null_buffers(), std::bind(&Client::event_cb, client, socket, act));
    }
    else if(act == CURL_POLL_INOUT)
    {
        socket->async_read_some(asio::null_buffers(), std::bind(&Client::event_cb, client, socket, act));
        socket->async_write_some(asio::null_buffers(), std::bind(&Client::event_cb, client, socket, act));
    }
}
 
void Client::addsock(curl_socket_t s, CURL *easy, int action, Client* client)
{
    /* fdp is used to store current action */
    int* fdp = (int*) calloc(sizeof(int), 1);
    setsock(fdp, s, easy, action, client);
    curl_multi_assign(client->m_multiHandle, s, fdp);
}

void Client::check_multi_info(Client* client)
{
    CURLMsg* msg;
    int msgs_left;

    while ((msg = curl_multi_info_read(client->m_multiHandle, &msgs_left)))
    {
        if(msg->msg == CURLMSG_DONE)
        {
            auto easy = msg->easy_handle;
            auto res = msg->data.result;
            
            ConnInfo* conn;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            curl_multi_remove_handle(client->m_multiHandle, easy);
            free(conn->url);
            curl_easy_cleanup(easy);
            free(conn);
        }
    }
}

/* Called by asio when there is an action on a socket */ 
void Client::event_cb(Client* client, asio::ip::tcp::socket* socket, int action)
{
    curl_multi_socket_action(client->m_multiHandle, socket->native_handle(), action, &client->m_stillRunning);
    check_multi_info(client);
 
    if (client->m_stillRunning <= 0)
    {
        client->m_timer.cancel();
    }
}
 
/* Called by asio when our timeout expires */ 
void Client::timer_cb(const std::error_code& error, Client* client)
{
    if (!error)
    {
        curl_multi_socket_action(client->m_multiHandle, CURL_SOCKET_TIMEOUT, 0, &client->m_stillRunning);
        check_multi_info(client);
    }
}

/* Update the event timer after curl_multi library calls */ 
int Client::multi_timer_cb(CURLM *multi, long timeout_ms, Client* client)
{
    /* cancel running timer */
    client->m_timer.cancel();
 
    if (timeout_ms > 0)
    {
        /* update timer */
        client->m_timer.expires_from_now(std::chrono::milliseconds(timeout_ms));
        client->m_timer.async_wait(std::bind(&timer_cb, std::placeholders::_1, client));
    }
    else
    {
        /* call timeout function immediately */
        std::error_code error; /*success*/
        timer_cb(error, client);
    }
 
  return 0;
}

}
}

#include "upnp/upnp.xml.parseutils.h"
#include "upnp/upnpdevice.h"

#include <cassert>
#include <pugixml.hpp>

namespace upnp
{
namespace xml
{

namespace
{

// enum class HostType
// {
//     Hostname,
//     IPv4Address
// };

// enum class PathType
// {
//     AbsolutePath,
//     RelativePath,
//     OpaquePart
// };

// enum class UriType
// {
//     Absolute,
//     Relative
// };

// struct Uri
// {
//     UriType type;
//     std::string scheme;
//     PathType pathType;
//     std::string pathQuery;
//     std::string fragment;
//     std::string hostport;
// };

std::string optional(const char* value)
{
    if (value)
    {
        return value;
    }

    return std::string();
}

std::string required(const char* value)
{
    if (value)
    {
        return value;
    }

    throw std::runtime_error("required xml node value not present");
}

// std::string parseScheme(const std::string& uri)
// {
//     std::string scheme;

//     auto iter = uri.find(':');
//     if (iter != std::string::npos)
//     {
//         if (iter == std::find_if(scheme.begin(), iter, [] (char c) { return !(isalnum(c) || c == '-' || c == '.'); }))
//         {
//             scheme = url.substr(0, iter);
//         }
//     }

//     return scheme;
// }

// int parseUri(const std::string& uri, size_t max, uri_type *out)
// {
//     int begin_path = 0;
//     size_t begin_hostport = (size_t)0;
//     size_t begin_fragment = (size_t)0;

//     begin_hostport = parse_scheme(in, max, &out->scheme);
//     if (begin_hostport) {
//         out->type = ABSOLUTE;
//         out->path_type = OPAQUE_PART;
//         begin_hostport++;
//     } else {
//         out->type = RELATIVE;
//         out->path_type = REL_PATH;
//     }
//     if (begin_hostport + (size_t)1 < max &&
//         in[begin_hostport] == '/' &&
//         in[begin_hostport + (size_t)1] == '/') {
//         begin_hostport += (size_t)2;
//         begin_path = parse_hostport(&in[begin_hostport],
//             &out->hostport);
//         if (begin_path >= 0) {
//             begin_path += (int)begin_hostport;
//         } else
//             return begin_path;
//     } else {
//         memset(&out->hostport, 0, sizeof(out->hostport));
//         begin_path = (int)begin_hostport;
//     }
//     begin_fragment = parse_uric(&in[begin_path],
//         max - (size_t)begin_path,
//         &out->pathquery) + (size_t)begin_path;
//     if (out->pathquery.size && out->pathquery.buff[0] == '/') {
//         out->path_type = ABS_PATH;
//     }
//     if (begin_fragment < max && in[begin_fragment] == '#') {
//         begin_fragment++;
//         parse_uric(&in[begin_fragment], max - begin_fragment,
//                &out->fragment);
//     } else {
//         out->fragment.buff = NULL;
//         out->fragment.size = (size_t)0;
//     }

//     return HTTP_SUCCESS;
// }




// std::string resolveUrl(const std::string& baseUrl, const std::string& relativeUrl)
// {
// 	std::string absoluteUrl;

// 	if (relativeUrl.empty())
//     {
// 		return absoluteUrl;
// 	}

//     uri_type base;
//     uri_type rel;

//     size_t i = (size_t)0;
//     char *finger = NULL;

//     char *last_slash = NULL;

//     char *out = NULL;

//     if( base_url && rel_url ) {
//         out =
//             ( char * )malloc( strlen( base_url ) + strlen( rel_url ) + (size_t)2 );
//     } else {
//         if( rel_url )
//             return strdup( rel_url );
//         else
//             return NULL;
//     }

//     if( out == NULL ) {
//         return NULL;
//     }
//     memset( out, 0, strlen( base_url ) + strlen( rel_url ) + (size_t)2 );

//     if( ( parse_uri( rel_url, strlen( rel_url ), &rel ) ) == HTTP_SUCCESS ) {

//         if( rel.type == ( enum uriType) ABSOLUTE ) {

//             strncpy( out, rel_url, strlen ( rel_url ) );
//         } else {

//             if( ( parse_uri( base_url, strlen( base_url ), &base ) ==
//                   HTTP_SUCCESS )
//                 && ( base.type == ( enum uriType ) ABSOLUTE ) ) {

//                 if( strlen( rel_url ) == (size_t)0 ) {
//                     strncpy( out, base_url, strlen ( base_url ) );
//                 } else {
//                     char *out_finger = out;
//                     assert( base.scheme.size + (size_t)1 /* ':' */ <= strlen ( base_url ) );
//                     memcpy( out, base.scheme.buff, base.scheme.size );
//                     out_finger += base.scheme.size;
//                     ( *out_finger ) = ':';
//                     out_finger++;

//                     if( rel.hostport.text.size > (size_t)0 ) {
//                         snprintf( out_finger, strlen( rel_url ) + (size_t)1,
//                                   "%s", rel_url );
//                     } else {
//                         if( base.hostport.text.size > (size_t)0 ) {
//                             assert( base.scheme.size + (size_t)1
//                                 + base.hostport.text.size + (size_t)2 /* "//" */ <= strlen ( base_url ) );
//                             memcpy( out_finger, "//", (size_t)2 );
//                             out_finger += 2;
//                             memcpy( out_finger, base.hostport.text.buff,
//                                     base.hostport.text.size );
//                             out_finger += base.hostport.text.size;
//                         }

//                         if( rel.path_type == ( enum pathType ) ABS_PATH ) {
//                             strncpy( out_finger, rel_url, strlen ( rel_url ) );

//                         } else {
//                             char temp_path = '/';

//                             if( base.pathquery.size == (size_t)0 ) {
//                                 base.pathquery.size = (size_t)1;
//                                 base.pathquery.buff = &temp_path;
//                             }

//                             assert( base.scheme.size + (size_t)1 + base.hostport.text.size + (size_t)2
//                                 + base.pathquery.size <= strlen ( base_url ) + (size_t)1 /* temp_path */);
//                             finger = out_finger;
//                             last_slash = finger;
//                             i = (size_t)0;
//                             while( ( i < base.pathquery.size ) &&
//                                    ( base.pathquery.buff[i] != '?' ) ) {
//                                 ( *finger ) = base.pathquery.buff[i];
//                                 if( base.pathquery.buff[i] == '/' )
//                                     last_slash = finger + 1;
//                                 i++;
//                                 finger++;

//                             }
//                             strncpy( last_slash, rel_url, strlen ( rel_url ) );
//                             if( remove_dots( out_finger,
//                                              strlen( out_finger ) ) !=
//                                 UPNP_E_SUCCESS ) {
//                                 free(out);
//                                 /* free(rel_url); */
//                                 return NULL;
//                             }
//                         }

//                     }
//                 }
//             } else {
//                 free(out);
//                 /* free(rel_url); */
//                 return NULL;
//             }
//         }
//     } else {
//         free(out);
//         /* free(rel_url); */
//         return NULL;
//     }

//     /* free(rel_url); */
//     return out;
// }

bool findAndParseService(const pugi::xml_node& node, const ServiceType serviceType, Device& device)
{
    bool found = false;
    auto base = device.m_baseURL.empty() ? device.m_location : device.m_baseURL;

    for (auto& serviceNode : node.children("serviceList"))
    {
        assert(serviceNode.name() == std::string("service"));

        Service service;
        service.m_type = serviceTypeUrnStringToService(required(serviceNode.child_value("serviceType")));
        if (service.m_type == serviceType)
        {
            service.m_id                    = required(serviceNode.child_value("serviceId"));
            std::string relControlURL       = required(serviceNode.child_value("controlURL"));
            std::string relEventURL         = required(serviceNode.child_value("eventSubURL"));
            std::string scpURL              = required(serviceNode.child_value("SCPDURL"));

//            char url[512];
//            int ret = UpnpResolveURL(base.c_str(), relControlURL.c_str(), url);
//            if (ret != UPNP_E_SUCCESS)
//            {
//                log::error("Error generating controlURL from {} and {}", base, relControlURL);
//            }
//            else
//            {
//                service.m_controlURL = url;
//            }

//            ret = UpnpResolveURL(base.c_str(), relEventURL.c_str(), url);
//            if (ret != UPNP_E_SUCCESS)
//            {
//                log::error("Error generating eventURL from {} and {}", base, relEventURL);
//            }
//            else
//            {
//                service.m_eventSubscriptionURL = url;
//            }
//
//            ret = UpnpResolveURL(base.c_str(), scpURL.c_str(), url);
//            if (ret != UPNP_E_SUCCESS)
//            {
//                log::error("Error generating eventURL from {} and {}", base, scpURL);
//            }
//            else
//            {
//                service.m_scpdUrl = url;
//            }

            device.m_services[serviceType] = service;
            found = true;
            break;
        }
    }

    return found;
}

}

void parseDeviceInfo(const std::string& xml, Device& device)
{
    pugi::xml_document doc;
    auto result = doc.load_buffer(xml.c_str(), xml.size());

    if (!result)
    {
        throw std::runtime_error("Failed to load device info xml");
    }

    auto deviceNode = doc.child("root").child("device");
    device.m_udn = required(deviceNode.child_value("UDN"));
    device.m_type = Device::stringToDeviceType(required(deviceNode.child_value("deviceType")));

    if (device.m_udn.empty())
    {
        return;
    }

    device.m_friendlyName   = required(deviceNode.child_value("friendlyName"));
    device.m_baseURL        = optional(deviceNode.child_value("URLBase"));
    device.m_relURL         = optional(deviceNode.child_value("presentationURL"));

    // char presURL[200];
    // int ret = UpnpResolveURL((device.m_baseURL.empty() ? device.m_baseURL.c_str() : device.m_location.c_str()), device.m_relURL.empty() ? nullptr : device.m_relURL.c_str(), presURL);
    // if (UPNP_E_SUCCESS == ret)
    // {
    //     device.m_presURL = presURL;
    // }

    if (device.m_type == DeviceType::MediaServer)
    {
        if (findAndParseService(doc, ServiceType::ContentDirectory, device))
        {
            // try to obtain the optional services
            findAndParseService(doc, ServiceType::AVTransport, device);
            findAndParseService(doc, ServiceType::ConnectionManager, device);
        }
    }
    else if (device.m_type == DeviceType::MediaRenderer)
    {
        if (findAndParseService(doc, ServiceType::RenderingControl, device) &&
            findAndParseService(doc, ServiceType::ConnectionManager, device))
        {
            // try to obtain the optional services
            findAndParseService(doc, ServiceType::AVTransport, device);
        }
    }
}

}
}

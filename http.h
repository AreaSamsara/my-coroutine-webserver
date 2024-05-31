#pragma once
#include <memory>
#include <map>
#include <boost/lexical_cast.hpp>

//#include "log.h"
//#include "singleton.h"
//#include "utility.h"
//
//using namespace LogSpace;
//using namespace SingletonSpace;
//using namespace UtilitySpace;

namespace HttpSpace
{
    using std::string;
    using std::map;
    using std::ostream;
    using std::shared_ptr;


	/* Request Methods */
#define HTTP_METHOD_MAP(XX)         \
  XX(0,  DELETE,      DELETE)       \
  XX(1,  GET,         GET)          \
  XX(2,  HEAD,        HEAD)         \
  XX(3,  POST,        POST)         \
  XX(4,  PUT,         PUT)          \
  /* pathological */                \
  XX(5,  CONNECT,     CONNECT)      \
  XX(6,  OPTIONS,     OPTIONS)      \
  XX(7,  TRACE,       TRACE)        \
  /* WebDAV */                      \
  XX(8,  COPY,        COPY)         \
  XX(9,  LOCK,        LOCK)         \
  XX(10, MKCOL,       MKCOL)        \
  XX(11, MOVE,        MOVE)         \
  XX(12, PROPFIND,    PROPFIND)     \
  XX(13, PROPPATCH,   PROPPATCH)    \
  XX(14, SEARCH,      SEARCH)       \
  XX(15, UNLOCK,      UNLOCK)       \
  XX(16, BIND,        BIND)         \
  XX(17, REBIND,      REBIND)       \
  XX(18, UNBIND,      UNBIND)       \
  XX(19, ACL,         ACL)          \
  /* subversion */                  \
  XX(20, REPORT,      REPORT)       \
  XX(21, MKACTIVITY,  MKACTIVITY)   \
  XX(22, CHECKOUT,    CHECKOUT)     \
  XX(23, MERGE,       MERGE)        \
  /* upnp */                        \
  XX(24, MSEARCH,     M-SEARCH)     \
  XX(25, NOTIFY,      NOTIFY)       \
  XX(26, SUBSCRIBE,   SUBSCRIBE)    \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)  \
  /* RFC-5789 */                    \
  XX(28, PATCH,       PATCH)        \
  XX(29, PURGE,       PURGE)        \
  /* CalDAV */                      \
  XX(30, MKCALENDAR,  MKCALENDAR)   \
  /* RFC-2068, section 19.6.1.2 */  \
  XX(31, LINK,        LINK)         \
  XX(32, UNLINK,      UNLINK)       \
  /* icecast */                     \
  XX(33, SOURCE,      SOURCE)       \

/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                 \
  XX(100, CONTINUE,                        Continue)                        \
  XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
  XX(102, PROCESSING,                      Processing)                      \
  XX(200, OK,                              OK)                              \
  XX(201, CREATED,                         Created)                         \
  XX(202, ACCEPTED,                        Accepted)                        \
  XX(203, NON_AUTHORITATIVE_INFORMATION,   Non-Authoritative Information)   \
  XX(204, NO_CONTENT,                      No Content)                      \
  XX(205, RESET_CONTENT,                   Reset Content)                   \
  XX(206, PARTIAL_CONTENT,                 Partial Content)                 \
  XX(207, MULTI_STATUS,                    Multi-Status)                    \
  XX(208, ALREADY_REPORTED,                Already Reported)                \
  XX(226, IM_USED,                         IM Used)                         \
  XX(300, MULTIPLE_CHOICES,                Multiple Choices)                \
  XX(301, MOVED_PERMANENTLY,               Moved Permanently)               \
  XX(302, FOUND,                           Found)                           \
  XX(303, SEE_OTHER,                       See Other)                       \
  XX(304, NOT_MODIFIED,                    Not Modified)                    \
  XX(305, USE_PROXY,                       Use Proxy)                       \
  XX(307, TEMPORARY_REDIRECT,              Temporary Redirect)              \
  XX(308, PERMANENT_REDIRECT,              Permanent Redirect)              \
  XX(400, BAD_REQUEST,                     Bad Request)                     \
  XX(401, UNAUTHORIZED,                    Unauthorized)                    \
  XX(402, PAYMENT_REQUIRED,                Payment Required)                \
  XX(403, FORBIDDEN,                       Forbidden)                       \
  XX(404, NOT_FOUND,                       Not Found)                       \
  XX(405, METHOD_NOT_ALLOWED,              Method Not Allowed)              \
  XX(406, NOT_ACCEPTABLE,                  Not Acceptable)                  \
  XX(407, PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   \
  XX(408, REQUEST_TIMEOUT,                 Request Timeout)                 \
  XX(409, CONFLICT,                        Conflict)                        \
  XX(410, GONE,                            Gone)                            \
  XX(411, LENGTH_REQUIRED,                 Length Required)                 \
  XX(412, PRECONDITION_FAILED,             Precondition Failed)             \
  XX(413, PAYLOAD_TOO_LARGE,               Payload Too Large)               \
  XX(414, URI_TOO_LONG,                    URI Too Long)                    \
  XX(415, UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          \
  XX(416, RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           \
  XX(417, EXPECTATION_FAILED,              Expectation Failed)              \
  XX(421, MISDIRECTED_REQUEST,             Misdirected Request)             \
  XX(422, UNPROCESSABLE_ENTITY,            Unprocessable Entity)            \
  XX(423, LOCKED,                          Locked)                          \
  XX(424, FAILED_DEPENDENCY,               Failed Dependency)               \
  XX(426, UPGRADE_REQUIRED,                Upgrade Required)                \
  XX(428, PRECONDITION_REQUIRED,           Precondition Required)           \
  XX(429, TOO_MANY_REQUESTS,               Too Many Requests)               \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   \
  XX(500, INTERNAL_SERVER_ERROR,           Internal Server Error)           \
  XX(501, NOT_IMPLEMENTED,                 Not Implemented)                 \
  XX(502, BAD_GATEWAY,                     Bad Gateway)                     \
  XX(503, SERVICE_UNAVAILABLE,             Service Unavailable)             \
  XX(504, GATEWAY_TIMEOUT,                 Gateway Timeout)                 \
  XX(505, HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      \
  XX(506, VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         \
  XX(507, INSUFFICIENT_STORAGE,            Insufficient Storage)            \
  XX(508, LOOP_DETECTED,                   Loop Detected)                   \
  XX(510, NOT_EXTENDED,                    Not Extended)                    \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

//HTTP方法枚举
enum HttpMethod
{
#define XX(num, name, string) name = num,
    HTTP_METHOD_MAP(XX)
#undef XX
    INVALID_METHOD
};

//HTTP状态枚举
enum class HttpStatus 
{
#define XX(code, name, desc) name = code,
    HTTP_STATUS_MAP(XX)
#undef XX
};


HttpMethod StringToHttpMethod(const string& method);
HttpMethod CharsTOHttpMethod(const char* method);
const char* HttpMethodToString(const HttpMethod& method);
const char* HttpStatusToString(const HttpStatus& status);

//忽略大小写比较字符串
class CaseInsensitiveLess
{
public:
    bool operator()(const string& lhs, const string& rhs)const;
};





template<class T>
bool check_getAs(const map<string, string, CaseInsensitiveLess>& map, const string& key, T& value, const T& default_value = T())
{
    auto iterator = map.find(key);
    if (iterator == map.end())
    {
        value = default_value;
        return false;
    }
    try
    {
        value = boost::lexical_cast<T>(iterator->second);
        return true;
    }
    catch (...)
    {
        value = default_value;
    }
    return false;
}

template<class T>
T getAs(const map<string, string, CaseInsensitiveLess>& map, const string& key, const T& default_value = T())
{
    auto iterator = map.find(key);
    if (iterator == map.end())
    {
        return default_value;
    }
    try
    {
        return boost::lexical_cast<T>(iterator->second);
    }
    catch (...)
    {
        return default_value;
    }
}


class HttpRequest
{
public:
    HttpRequest(const uint8_t version = 0x11, const bool close = true);

    HttpMethod getMethod()const { return m_method; }
    uint8_t getVersion()const { return m_version; }
    bool isClose() const { return m_close; }
    const string& getPath()const { return m_path; }
    const string& getQuery()const { return m_query; }
    const string& getBody()const { return m_body; }
    map<string, string, CaseInsensitiveLess> getHeaders()const { return m_headers; }
    map<string, string, CaseInsensitiveLess> getParameters()const { return m_parameters; }
    map<string, string, CaseInsensitiveLess> getCookie()const { return m_cookies; }

    void setMethod(const HttpMethod method) { m_method = method; }
    void setVersion(const uint8_t version) { m_version = version; }
    void setClose(const bool close) { m_close = close; }
    void setPath(const string& path) { m_path = path; }
    void setQuery(const string& query) { m_query = query; }
    void setBody(const string& body) { m_body = body; }
    void setFragment(const string& fragment) { m_fragment = fragment; }
    void setHeaders(const map<string, string, CaseInsensitiveLess>& headers) { m_headers = headers; }
    void setParameters(const map<string, string, CaseInsensitiveLess>& parameters) { m_parameters = parameters; }
    void setCookies(const map<string, string, CaseInsensitiveLess>& cookies) { m_cookies = cookies; }

    void setHeader(const string& key,const string& header);
    void setParameter(const string& key, const string& parameter);
    void setCookie(const string& key, const string& cookie);

    string getHeader(const string& key, const string& default_header = "") const;
    string getParameter(const string& key, const string& default_parameter = "")const;
    string getCookie(const string& key, const string& default_cookie = "")const;

    void deleteHeader(const string& key);
    void deleteParameter(const string& key);
    void deleteCookie(const string& key);

    bool hasHeader(const string& key, string* header = nullptr);
    bool hasParameter(const string& key, string* parameter = nullptr);
    bool hasCookie(const string& key, string* cookie = nullptr);

    ostream& dump(ostream& os)const;
    string toString()const;

    template<class T>
    bool check_getHeaderAs(const string& key, T& value, const T& default_value = T())
    {
        return check_getHeaderAs(m_headers, key, value, default_value);
    }
    template<class T>
    T getHeaderAs(const string& key, const T& default_value = T())
    {
        return getAs(m_headers, key, default_value);
    }

    template<class T>
    bool check_getParameterAs(const string& key, T& value, const T& default_value = T())
    {
        return check_getHeaderAs(m_parameters, key, value, default_value);
    }
    template<class T>
    T getParamAs(const string& key, const T& default_value = T())
    {
        return getAs(m_parameters, key, default_value);
    }

    template<class T>
    bool check_getCookieAs(const string& key, T& value, const T& default_value = T())
    {
        return check_getHeaderAs(m_cookies, key, value, default_value);
    }
    template<class T>
    T getCookieAs(const string& key, const T& default_value = T())
    {
        return getAs(m_cookies, key, default_value);
    }
private:
    HttpMethod m_method;
    uint8_t m_version;
    bool m_close;

    string m_path;
    string m_query;
    string m_body;
    string m_fragment;

    map<string, string, CaseInsensitiveLess> m_headers;
    map<string, string, CaseInsensitiveLess> m_parameters;
    map<string, string, CaseInsensitiveLess> m_cookies;
};



class HttpResponse
{
public:
    HttpResponse(const uint8_t version = 0x11, const bool close = true);

    HttpStatus getStatus()const { return m_status; }
    uint8_t getVersion()const { return m_version; }
    bool isClose() const { return m_close; }
    const string& getBody()const { return m_body; }
    const string& getReason()const { return m_reason; }
    map<string, string, CaseInsensitiveLess> getHeaders()const { return m_headers; }

    void setStatus(const HttpStatus status) { m_status = status; }
    void setVersion(const uint8_t version) { m_version = version; }
    void setClose(const bool close) { m_close = close; }
    void setBody(const string& body) { m_body = body; }
    void setReason(const string& reason) { m_reason = reason; }
    void setHeaders(const map<string, string, CaseInsensitiveLess>& headers) { m_headers = headers; }


    void setHeader(const string& key, const string& header);
    string getHeader(const string& key, const string& default_header = "") const;
    void deleteHeader(const string& key);
    bool hasHeader(const string& key, string* header = nullptr);

    ostream& dump(ostream& os)const;
    string toString()const;

    template<class T>
    bool check_getHeaderAs(const string& key, T& value, const T& default_value = T())
    {
        return check_getHeaderAs(m_headers, key, value, default_value);
    }
    template<class T>
    T getHeaderAs(const string& key, const T& default_value = T())
    {
        return getAs(m_headers, key, default_value);
    }
private:
    HttpStatus m_status;
    uint8_t m_version;
    bool m_close;
    string m_body;
    string m_reason;
    map<string, string, CaseInsensitiveLess> m_headers;
};






}
#include "http.h"
#include <sstream>

namespace HttpSpace
{
	using std::stringstream;


	HttpMethod StringToHttpMethod(const string& method)
	{
#define XX(num, name, string) \
    if(strcmp(#string, method.c_str()) == 0) { \
        return HttpMethod::name; \
    }
		HTTP_METHOD_MAP(XX);
#undef XX
		return HttpMethod::INVALID_METHOD;
	}
	HttpMethod CharsTOHttpMethod(const char* method)
	{
#define XX(num, name, string) \
    if(strncmp(#string, method, strlen(#string)) == 0) { \
        return HttpMethod::name; \
    }
		HTTP_METHOD_MAP(XX);
#undef XX
		return HttpMethod::INVALID_METHOD;
	}



	static const char* s_method_string[] = 
	{
#define XX(num, name, string) #string,
	HTTP_METHOD_MAP(XX)
#undef XX
	};

	const char* HttpMethodToString(const HttpMethod& method)
	{
		uint32_t idx = (uint32_t)method;
		if (idx >= (sizeof(s_method_string) / sizeof(s_method_string[0]))) {
			return "<unknown>";
		}
		return s_method_string[idx];
	}

	const char* HttpStatusToString(const HttpStatus& status)
	{
		switch (status) {
#define XX(code, name, msg) \
        case HttpStatus::name: \
            return #msg;
			HTTP_STATUS_MAP(XX);
#undef XX
		default:
			return "<unknown>";
		}
	}


    //class CaseInsensitiveLess:public
	bool CaseInsensitiveLess::operator()(const string& lhs, const string& rhs)const
	{
		//按字典顺序比较两个字符串，不区分大小写
		return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
	}



	//class HttpRequest:public
	HttpRequest::HttpRequest(const uint8_t version, const bool close) 
		:m_method(HttpMethod::GET),m_version(version),m_is_close(close),m_path("/")
    {

    }

	void HttpRequest::setHeader(const string& key, const string& header)
	{
		m_headers[key] = header;
	}
	void HttpRequest::setParameter(const string& key, const string& parameter)
	{
		m_parameters[key] = parameter;
	}
	void HttpRequest::setCookie(const string& key, const string& cookie)
	{
		m_cookies[key] = cookie;
	}

	string HttpRequest::getHeader(const string& key, const string& default_header) const
	{
		auto iterator = m_headers.find(key);
		return iterator == m_headers.end() ? default_header : iterator->second;
	}
	string HttpRequest::getParameter(const string& key, const string& default_parameter)const
	{
		auto iterator = m_parameters.find(key);
		return iterator == m_parameters.end() ? default_parameter : iterator->second;
	}
	string HttpRequest::getCookie(const string& key, const string& default_cookie)const
	{
		auto iterator = m_cookies.find(key);
		return iterator == m_cookies.end() ? default_cookie : iterator->second;
	}

	void HttpRequest::deleteHeader(const string& key)
	{
		m_headers.erase(key);
	}
	void HttpRequest::deleteParameter(const string& key)
	{
		m_parameters.erase(key);
	}
	void HttpRequest::deleteCookie(const string& key)
	{
		m_cookies.erase(key);
	}

	bool HttpRequest::hasHeader(const string& key, string* header)
	{
		auto iterator = m_headers.find(key);
		if (iterator == m_headers.end())
		{
			return false;
		}
		if (header)
		{
			*header = iterator->second;
		}
		return true;
	}
	bool HttpRequest::hasParameter(const string& key, string* parameter)
	{
		auto iterator = m_parameters.find(key);
		if (iterator == m_parameters.end())
		{
			return false;
		}
		if (parameter)
		{
			*parameter = iterator->second;
		}
		return true;
	}
	bool HttpRequest::hasCookie(const string& key, string* cookie)
	{
		auto iterator = m_cookies.find(key);
		if (iterator == m_cookies.end())
		{
			return false;
		}
		if (cookie)
		{
			*cookie = iterator->second;
		}
		return true;
	}

	ostream& HttpRequest::dump(ostream& os)const
	{
		//GET /url HTTP/1.1
		//Host: www.baidu.com
		//...
		//...

		//第一行
		os << HttpMethodToString(m_method) << " "
			<< m_path
			<< (m_query.empty() ? "" : "?")
			<< m_query
			<< (m_fragment.empty() ? "" : "#")
			<< m_fragment
			<< " HTTP/"
			<< ((uint32_t)(m_version >> 4))
			<< "."
			<< ((uint32_t)(m_version & 0x0F))
			<< "\r\n";

		os << "connection: " << (m_is_close ? "close" : "keep-alive") << "\r\n";

		//请求头部
		for (auto& header : m_headers)
		{
			if (strcasecmp(header.first.c_str(), "connection") != 0)
			{
				os << header.first << ":" << header.second << "\r\n";
			}
		}

		//请求体
		if (!m_body.empty())
		{
			os << "content-length: " << m_body.size() << "\r\n\r\n"
				<< m_body;
		}
		else
		{
			os << "\r\n";
		}

		return os;
	}

	string HttpRequest::toString() const
	{
		stringstream ss;
		dump(ss);
		return ss.str();
	}



	//class HttpResponse:public
	HttpResponse::HttpResponse(const uint8_t version, const bool close)
		:m_status(HttpStatus::OK), m_version(version), m_is_close(close) {}

	void HttpResponse::setHeader(const string& key, const string& header)
	{
		m_headers[key] = header;
	}
	string HttpResponse::getHeader(const string& key, const string& default_header) const
	{
		auto iterator = m_headers.find(key);
		return iterator == m_headers.end() ? default_header : iterator->second;
	}
	void HttpResponse::deleteHeader(const string& key)
	{
		m_headers.erase(key);
	}
	bool HttpResponse::hasHeader(const string& key, string* header)
	{
		auto iterator = m_headers.find(key);
		if (iterator == m_headers.end())
		{
			return false;
		}
		if (header)
		{
			*header = iterator->second;
		}
		return true;
	}

	ostream& HttpResponse::dump(ostream& os)const
	{
		os << "HTTP/"
			<< ((uint32_t)(m_version >> 4))
			<< "."
			<< ((uint32_t)(m_version & 0x0F))
			<< " "
			<< (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
			<< "\r\n";

		for (auto& header : m_headers)
		{
			if (strcasecmp(header.first.c_str(), "connection") != 0)
			{
				os << header.first << ": " << header.second << "\r\n";
			}
		}

		os << "connection: " << (m_is_close ? "close" : "keep-alive") << "\r\n";

		if (!m_body.empty())
		{
			os << "content-length: " << m_body.size() << "\r\n\r\n"
				<< m_body;
		}
		else
		{
			os << "\r\n";
		}
		return os;
	}
	string HttpResponse::toString()const
	{
		stringstream ss;
		dump(ss);
		return ss.str();
	}
}
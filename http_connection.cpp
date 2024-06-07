#include "http_connection.h"
#include "http_parser.h"
#include "log.h"
#include "utility.h"
#include "singleton.h"

namespace HttpConnectionSpace
{
	using namespace LogSpace;
	using namespace UtilitySpace;
	using namespace SingletonSpace;
	using std::stringstream;
	using std::make_shared;

	//class HttpResult:public
	HttpResult::HttpResult(const int result, shared_ptr<HttpResponse> response, const string& error)
		:m_result(result), m_response(response), m_error(error) {}

	string HttpResult::toString()const
	{
		stringstream ss;
		ss << "[HttpResult result=" << m_result
			<< " error=" << m_error
			<< " response=" << (m_response ? m_response->toString() : "nullptr")
			<< "]";
		return ss.str();
	}




	//class HttpConnection:public
	HttpConnection::HttpConnection(shared_ptr<Socket> socket, const bool is_socket_owner)
		:SocketStream(socket, is_socket_owner) {}


	shared_ptr<HttpResponse> HttpConnection::receiveResponse()
	{
		shared_ptr<HttpResponseParser> parser(new HttpResponseParser);
		uint64_t buffer_size = HttpResponseParser::GetHttp_response_buffer_size();
		//uint64_t buffer_size = 100;

		shared_ptr<char> buffer(new char[buffer_size+1], [](char* ptr) {delete[]ptr; });	//多加一个字节专门去放那个0？？？

		char* data = buffer.get();
		int offset = 0;

		while (true)
		{
			int length = read(data + offset, buffer_size - offset);
			if (length <= 0)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "length <= 0";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);

				close();
				return nullptr;
			}
			length += offset;

			data[length] = '\0';

			int parser_length = parser->execute(data, length,false);
			if (parser->hasError())
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "parser->hasError()";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);

				close();
				return nullptr;
			}

			offset = length - parser_length;

			if (offset == buffer_size)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "offset == buffer_size";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);

				close();
				return nullptr;
			}
			if (parser->isFinished())
			{
				break;
			}
		}

		auto client_parser = parser->getParser();
		if (client_parser.chunked)
		{
			string body;
			int length = offset;

			do
			{
				do
				{
					int return_value = read(data + length, buffer_size - length);
					if (return_value <= 0)
					{
						close();
						return nullptr;
					}
					length += return_value;
					data[length] = '\0';

					size_t length_parsed = parser->execute(data, length, true);

					if (parser->hasError())
					{
						close();
						return nullptr;
					}
					length -= length_parsed;
					//如果压根没解读，直接返回nullptr
					if (length == buffer_size)
					{
						close();
						return nullptr;
					}
				} while (parser->isFinished());

				length -= 2;

				if (client_parser.content_len <= length)
				{
					body.append(data, client_parser.content_len);
					memmove(data, data + client_parser.content_len, length - client_parser.content_len);
					length -= client_parser.content_len;
				}
				else
				{
					body.append(data, length);
					int left = client_parser.content_len - length;
					while (left > 0)
					{
						int return_value = read(data, left > buffer_size ? buffer_size : left);
						if (return_value <= 0)
						{
							close();
							return nullptr;
						}
						body.append(data, return_value);
						left -= return_value;
					}
					length = 0;
				}

			} while (!client_parser.chunks_done);
			parser->getResponse()->setBody(body);
		}
		else
		{
			uint64_t content_length = parser->getContentLength();
			if (content_length > 0)
			{
				string body;
				body.resize(content_length);

				int length = 0;
				if (content_length >= offset)
				{
					memcpy(&body[0], data, offset);
					length = offset;
				}
				else
				{
					memcpy(&body[0], data, content_length);
					length = content_length;
				}

				content_length -= offset;
				if (content_length > 0)
				{
					if (read_fixed_size(&body[length], content_length) <= 0)
					{
						close();
						return nullptr;
					}
				}
				parser->getResponse()->setBody(body);
			}
		}	

		return parser->getResponse();
	}

	int HttpConnection::sendRequest(shared_ptr<HttpRequest> request)
	{
		stringstream ss;
		ss << request->toString();
		string data = ss.str();
		return write_fixed_size(data.c_str(), data.size());
	}

	//class HttpConnection:public static
	shared_ptr<HttpResult> HttpConnection::DoRequest(HttpMethod method, const string& url, const uint64_t timeout,
		const map<string, string>& headers , const string& body)
	{
		shared_ptr<Uri> uri = Uri::Create(url);
		if (!uri)
		{
			return make_shared<HttpResult>(HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
		}
		return DoRequest(method, uri, timeout, headers, body);
	}

	shared_ptr<HttpResult> HttpConnection::DoRequest(HttpMethod method, shared_ptr<Uri> uri, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		shared_ptr<HttpRequest> request(new HttpRequest());
		request->setPath(uri->getPath());
		request->setQuery(uri->getQuery());
		request->setFragment(uri->getFragment());
		request->setMethod(method);

		bool has_host = false;
		for (auto& header : headers)
		{
			if (strcasecmp(header.first.c_str(), "connection") == 0)
			{
				if (strcasecmp(header.second.c_str(), "keep-alive") == 0)
				{
					request->setClose(false);
				}
				continue;
			}

			if (!has_host && strcasecmp(header.first.c_str(), "host") == 0)
			{
				has_host = !header.second.empty();
			}

			request->setHeader(header.first,header.second);
		}
		
		if (!has_host)
		{
			request->setHeader("Host", uri->getHost());
		}

		request->setBody(body);
		return DoRequest(request, uri, timeout);
	}

	shared_ptr<HttpResult> HttpConnection::DoRequest(shared_ptr<HttpRequest> request, shared_ptr<Uri> uri, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		shared_ptr<Address> address = uri->createAddress();
		if (!address)
		{
			return make_shared<HttpResult>(HttpResult::Error::INVALID_HOST, nullptr, "invalid host: " + uri->getHost());
		}

		shared_ptr<Socket> socket(new Socket(Socket::FamilyType(address->getFamily()), Socket::SocketType::TCP, 0));
		if (!socket)
		{
			return make_shared<HttpResult>(HttpResult::Error::CREATE_SOCKET_FAIL, nullptr, "create socket fail,address="
				+ address->toString() + " errno=" + to_string(errno) + " strerror" + string(strerror(errno)));
		}

		if(!socket->connect(address))
		{
			return make_shared<HttpResult>(HttpResult::Error::CONNECT_FAIL, nullptr, "connect fail: " + address->toString());
		}


		socket->setReceive_timeout(timeout);

		shared_ptr<HttpConnection> connection(new HttpConnection(socket));
		int return_value = connection->sendRequest(request);
		if (return_value == 0)
		{
			return make_shared<HttpResult>(HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "send fail: close by peer,address: " + address->toString());
		}
		else if (return_value < 0)
		{
			return make_shared<HttpResult>(HttpResult::Error::SEND_SOCKET_ERROR, nullptr,
				"send fail: socket error,errno=" + to_string(errno) + " strerror=" + string(strerror(errno)));
		}

		auto response = connection->receiveResponse();
		if (!response)
		{
			return make_shared<HttpResult>(HttpResult::Error::TIMEOUT, nullptr,
				"receive response time out: " + address->toString() + " timeout=" + to_string(timeout));
		}


		return make_shared<HttpResult>(HttpResult::Error::OK, response, "ok");
	}

	
	shared_ptr<HttpResult> HttpConnection::DoGet(const string& url, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		shared_ptr<Uri> uri = Uri::Create(url);
		if (!uri)
		{
			return make_shared<HttpResult>(HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
		}
		return DoGet(uri, timeout, headers, body);
	}

	shared_ptr<HttpResult> HttpConnection::DoGet(shared_ptr<Uri> uri, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		return DoRequest(HttpMethod::GET, uri, timeout, headers, body);
	}

	
	shared_ptr<HttpResult> HttpConnection::DoPost(const string& url, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		shared_ptr<Uri> uri = Uri::Create(url);
		if (!uri)
		{
			return make_shared<HttpResult>(HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url);
		}
		return DoPost(uri, timeout, headers, body);
	}
	shared_ptr<HttpResult> HttpConnection::DoPost(shared_ptr<Uri> uri, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		return DoRequest(HttpMethod::POST,uri,timeout,headers,body);
	}






	
	//class HttpConnectionPool:public
	HttpConnectionPool::HttpConnectionPool(const string& host, const string& vhost, const uint32_t port, const uint32_t max_size,
		const uint32_t max_alive_time, const uint32_t max_request)
		:m_host(host), m_vhost(vhost), m_port(port), m_max_size(max_size), m_max_alive_time(max_alive_time), 
		m_max_request(max_request) {}

	shared_ptr<HttpConnection> HttpConnectionPool::getConnection()
	{
		uint64_t now = GetCurrentMS();
		vector<HttpConnection*> invalid_connections;
		HttpConnection* ptr = nullptr;

		{
			//先监视互斥锁，保护
			ScopedLock<Mutex> lock(m_mutex);

			while (!m_connections.empty())
			{
				auto connection = *m_connections.begin();
				m_connections.pop_front();
				if (!connection->getSocket() || !connection->getSocket()->isConnected())
				{
					invalid_connections.push_back(connection);
					continue;
				}
				if (connection->getCreate_time() + m_max_alive_time > now)
				{
					invalid_connections.push_back(connection);
					continue;
				}
				ptr = connection;
				break;
			}
		}

		for (auto invalid_connection : invalid_connections)
		{
			delete invalid_connection;
		}
		m_total -= invalid_connections.size();

		if (ptr == nullptr)
		{
			shared_ptr<IPAddress> address = Address::LookupAnyIPAddress(m_host);
			if (!address)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "get address fail,host: " << m_host;
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
				return nullptr;
			}

			address->setPort(m_port);

			shared_ptr<Socket> socket(new Socket(Socket::FamilyType(address->getFamily()), Socket::SocketType::TCP, 0));
			if (!socket)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "create socket fail,address=" << address->toString();
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
				return nullptr;
			}
			if (!socket->connect(address))
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "socket connect fail,address=" << address->toString();
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
				return nullptr;
			}

			ptr = new HttpConnection(socket);
			++m_total;
		}

		return shared_ptr<HttpConnection>(ptr, bind(&HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));
	}

	shared_ptr<HttpResult> HttpConnectionPool::doRequest(HttpMethod method, const string& url, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		shared_ptr<HttpRequest> request(new HttpRequest());
		request->setPath(url);
		request->setMethod(method);
		request->setClose(false);

		bool has_host = false;
		for (auto& header : headers)
		{
			if (strcasecmp(header.first.c_str(), "connection") == 0)
			{
				if (strcasecmp(header.second.c_str(), "keep-alive") == 0)
				{
					request->setClose(false);
				}
				continue;
			}

			if (!has_host && strcasecmp(header.first.c_str(), "host") == 0)
			{
				has_host = !header.second.empty();
			}

			request->setHeader(header.first, header.second);
		}

		if (!has_host)
		{
			if (m_vhost.empty())
			{
				request->setHeader("host", m_host);
			}
			else
			{
				request->setHeader("host", m_vhost);
			}
		}

		request->setBody(body);
		return doRequest(request, timeout);
	}

	shared_ptr<HttpResult> HttpConnectionPool::doRequest(HttpMethod method, shared_ptr<Uri> uri, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		stringstream ss;
		ss << uri->getPath()
			<< (uri->getQuery().empty() ? "" : "?")
			<< uri->getQuery()
			<< (uri->getFragment().empty() ? "" : "?")
			<< uri->getFragment();
		return doRequest(method,ss.str(), timeout, headers, body);
	}
	shared_ptr<HttpResult> HttpConnectionPool::doRequest(shared_ptr<HttpRequest> request, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		auto connection = getConnection();
		if (!connection)
		{
			return make_shared<HttpResult>(HttpResult::Error::POOL_GET_CONNECTION_FAIL, nullptr,
				"pool get connection fail,host: " + m_host + " port: " + to_string(m_port));
		}
		
		auto socket = connection->getSocket();
		if (!socket)
		{
			return make_shared<HttpResult>(HttpResult::Error::POOL_INVALID_CONNECTION, nullptr,
				"pool invalid connection,host: " + m_host + " port: " + to_string(m_port));
		}


		socket->setReceive_timeout(timeout);

		int return_value = connection->sendRequest(request);
		if (return_value == 0)
		{
			return make_shared<HttpResult>(HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr,
				"send fail: close by peer,address: " + socket->getRemote_address()->toString());
		}
		else if (return_value < 0)
		{
			return make_shared<HttpResult>(HttpResult::Error::SEND_SOCKET_ERROR, nullptr,
				"send fail: socket error,errno=" + to_string(errno) + " strerror=" + string(strerror(errno)));
		}

		auto response = connection->receiveResponse();
		if (!response)
		{
			return make_shared<HttpResult>(HttpResult::Error::TIMEOUT, nullptr,
				"receive response time out: " + socket->getRemote_address()->toString() + " timeout=" + to_string(timeout));
		}


		return make_shared<HttpResult>(HttpResult::Error::OK, response, "ok");
	}


	shared_ptr<HttpResult> HttpConnectionPool::doGet(const string& url, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		return doRequest(HttpMethod::GET, url, timeout, headers, body);
	}
	shared_ptr<HttpResult> HttpConnectionPool::doGet(shared_ptr<Uri> uri, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		stringstream ss;
		ss << uri->getPath()
			<< (uri->getQuery().empty() ? "" : "?")
			<< uri->getQuery()
			<< (uri->getFragment().empty() ? "" : "?")
			<< uri->getFragment();
		return doGet(ss.str(), timeout, headers, body);
	}

	shared_ptr<HttpResult> HttpConnectionPool::doPost(const string& url, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		return doRequest(HttpMethod::POST, url, timeout, headers, body);
	}
	shared_ptr<HttpResult> HttpConnectionPool::doPost(shared_ptr<Uri> uri, const uint64_t timeout,
		const map<string, string>& headers, const string& body)
	{
		stringstream ss;
		ss << uri->getPath()
			<< (uri->getQuery().empty() ? "" : "?")
			<< uri->getQuery()
			<< (uri->getFragment().empty() ? "" : "?")
			<< uri->getFragment();
		return doPost(ss.str(), timeout, headers, body);
	}


	//class HttpConnectionPool:private static
	void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool)
	{
		ptr->setRequest_count(ptr->getRequest_count() + 1);

		if (!ptr->getSocket()->isConnected() 
			|| ptr->getCreate_time() + pool->m_max_alive_time >= GetCurrentMS() 
			|| ptr->getRequest_count() >= pool->m_max_request)
		{
			delete ptr;
			--pool->m_total;
		}
		else
		{
			//先监视互斥锁，保护
			ScopedLock<Mutex> lock(pool->m_mutex);
			pool->m_connections.push_back(ptr);
		}
	}
}
#pragma once
#include <atomic>
#include <list>
#include "io/socket-stream.h"
#include "http/http.h"
#include "http/parser/uri.h"
#include "concurrent/mutex.h"

namespace HttpConnectionSpace
{
	using namespace SocketStreamSpace;
	using namespace HttpSpace;
	using namespace UriSpace;
	using namespace MutexSpace;
	using std::atomic;
	using std::list;

	class HttpResult
	{
	public:
		enum Error
		{
			OK = 0,
			INVALID_URL = 1,
			INVALID_HOST = 2,
			CREATE_SOCKET_FAIL = 3,
			CONNECT_FAIL = 4,
			SEND_CLOSE_BY_PEER = 5,
			TIMEOUT = 7,
			SEND_SOCKET_ERROR = 6,
			POOL_GET_CONNECTION_FAIL = 7,
			POOL_INVALID_CONNECTION = 8
		};

	public:
		HttpResult(const int result, shared_ptr<HttpResponse> response, const string &error);

		string toString() const;

	public:
		int m_result;
		shared_ptr<HttpResponse> m_response;
		string m_error;
	};

	class HttpConnection : public SocketStream
	{
	public:
		HttpConnection(shared_ptr<Socket> socket, const bool owner = true);

		shared_ptr<HttpResponse> receiveResponse();
		int sendRequest(shared_ptr<HttpRequest> request);

		uint64_t getCreate_time() const { return m_create_time; }
		uint64_t getRequest_count() const { return m_request_count; }
		void setRequest_count(const uint64_t request_count) { m_request_count = request_count; }

	public:
		static shared_ptr<HttpResult> DoRequest(HttpMethod method, const string &url, const uint64_t timeout,
												const map<string, string> &headers = {}, const string &body = "");
		static shared_ptr<HttpResult> DoRequest(HttpMethod method, shared_ptr<Uri> uri, const uint64_t timeout,
												const map<string, string> &headers = {}, const string &body = "");
		static shared_ptr<HttpResult> DoRequest(shared_ptr<HttpRequest> request, shared_ptr<Uri> uri, const uint64_t timeout,
												const map<string, string> &headers = {}, const string &body = "");

		static shared_ptr<HttpResult> DoGet(const string &url, const uint64_t timeout,
											const map<string, string> &headers = {}, const string &body = "");
		static shared_ptr<HttpResult> DoGet(shared_ptr<Uri> uri, const uint64_t timeout,
											const map<string, string> &headers = {}, const string &body = "");

		static shared_ptr<HttpResult> DoPost(const string &url, const uint64_t timeout,
											 const map<string, string> &headers = {}, const string &body = "");
		static shared_ptr<HttpResult> DoPost(shared_ptr<Uri> uri, const uint64_t timeout,
											 const map<string, string> &headers = {}, const string &body = "");

	private:
		uint64_t m_create_time = 0;
		uint64_t m_request_count = 0;
	};

	class HttpConnectionPool
	{
	public:
		HttpConnectionPool(const string &host, const string &vhost, const uint32_t port, const uint32_t max_size,
						   const uint32_t max_alive_time, const uint32_t max_request);
		shared_ptr<HttpConnection> getConnection();

		shared_ptr<HttpResult> doRequest(HttpMethod method, const string &url, const uint64_t timeout,
										 const map<string, string> &headers = {}, const string &body = "");
		shared_ptr<HttpResult> doRequest(HttpMethod method, shared_ptr<Uri> uri, const uint64_t timeout,
										 const map<string, string> &headers = {}, const string &body = "");
		shared_ptr<HttpResult> doRequest(shared_ptr<HttpRequest> request, const uint64_t timeout,
										 const map<string, string> &headers = {}, const string &body = "");

		shared_ptr<HttpResult> doGet(const string &url, const uint64_t timeout,
									 const map<string, string> &headers = {}, const string &body = "");
		shared_ptr<HttpResult> doGet(shared_ptr<Uri> uri, const uint64_t timeout,
									 const map<string, string> &headers = {}, const string &body = "");

		shared_ptr<HttpResult> doPost(const string &url, const uint64_t timeout,
									  const map<string, string> &headers = {}, const string &body = "");
		shared_ptr<HttpResult> doPost(shared_ptr<Uri> uri, const uint64_t timeout,
									  const map<string, string> &headers = {}, const string &body = "");

	private:
		static void ReleasePtr(HttpConnection *ptr, HttpConnectionPool *pool);

	private:
		string m_host;
		string m_vhost;
		uint32_t m_port;
		uint32_t m_max_size;
		uint32_t m_max_alive_time;
		uint32_t m_max_request;
		// 互斥锁
		Mutex m_mutex;
		list<HttpConnection *> m_connections;
		atomic<int32_t> m_total = {0};
	};
}
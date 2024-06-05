#pragma once
#include <unordered_map>
#include "http.h"
#include "http_session.h"
#include "mutex.h"

namespace ServletSpace
{
	using namespace HttpSpace;
	using namespace HttpSessionSpace;
	using namespace MutexSpace;
	using std::function;
	using std::unordered_map;


	class Servlet
	{
	public:
		Servlet(const string& name);
		virtual ~Servlet() {}
		virtual int32_t handle(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response, 
			shared_ptr<HttpSession> session) = 0;
		virtual const string& getName()const { return m_name; }

	protected:
		string m_name;
	};


	class FunctionServlet :public Servlet
	{
	public:
		FunctionServlet(function<int32_t(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
			shared_ptr<HttpSession> session)> callback);
		int32_t handle(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
			shared_ptr<HttpSession> session)override;
	private:
		function<int32_t(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
			shared_ptr<HttpSession> session)> m_callback;
	};


	class ServletDispatch :public Servlet
	{
	public:
		ServletDispatch();

		void addServlet(const string& uri, shared_ptr<Servlet> servlet);
		void addServlet(const string& uri, function<int32_t(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
			shared_ptr<HttpSession> session)> callback);
		void addGlobServlet(const string& uri, shared_ptr<Servlet> servlet);
		void addGlobServlet(const string& uri, function<int32_t(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
			shared_ptr<HttpSession> session)> callback);

		void deleteServlet(const string& uri);
		void deleteGlobServlet(const string& uri);

		shared_ptr<Servlet> getDefault_servlet(const string& uri)const { return m_default_servlet; }
		void setDefault_servlet(shared_ptr<Servlet> default_servlet) { m_default_servlet = default_servlet; }

		shared_ptr<Servlet> getServlet(const string& uri);
		shared_ptr<Servlet> getGlobServlet(const string& uri);

		shared_ptr<Servlet> getMatched_servlet(const string& uri);

		int32_t handle(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
			shared_ptr<HttpSession> session)override;
	private:
		//uri(/test/xxx) -> servlet	¾«×¼Æ¥Åä
		unordered_map<string, shared_ptr<Servlet>> m_datas;
		//uri(/test/*) -> servlet	Ä£ºýÆ¥Åä
		vector<pair<string, shared_ptr<Servlet>>> m_globs;
		shared_ptr<Servlet> m_default_servlet;
		//»¥³âËø£¨¶Á/Ð´Ëø£©
		Mutex_Read_Write m_mutex;
	};

	class NotFoundServlet :public Servlet
	{
	public:
		NotFoundServlet();

		int32_t handle(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
			shared_ptr<HttpSession> session)override;
	private:
		string m_response_body;
	};
}
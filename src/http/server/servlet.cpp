#include "http/server/servlet.h"

#include <fnmatch.h>

namespace ServletSpace
{
	using std::make_pair;

	// class Servlet:public
	Servlet::Servlet(const string &name) : m_name(name) {}

	// class FunctionServlet:public
	FunctionServlet::FunctionServlet(function<int32_t(shared_ptr<HttpRequest> request,
													  shared_ptr<HttpResponse> response, shared_ptr<HttpSession> session)>
										 callback)
		: Servlet("FunctionServlet"), m_callback(callback) {}

	int32_t FunctionServlet::handle(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
									shared_ptr<HttpSession> session)
	{
		return m_callback(request, response, session);
	}

	// class ServletDispatch:public
	ServletDispatch::ServletDispatch() : Servlet("ServletDispatch"), m_default_servlet(new NotFoundServlet()) {}

	void ServletDispatch::addServlet(const string &uri, shared_ptr<Servlet> servlet)
	{
		// �ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		m_datas[uri] = servlet;
	}

	void ServletDispatch::addServlet(const string &uri, function<int32_t(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
																		 shared_ptr<HttpSession> session)>
															callback)
	{
		// �ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		m_datas[uri].reset(new FunctionServlet(callback));
	}

	void ServletDispatch::addGlobServlet(const string &uri, shared_ptr<Servlet> servlet)
	{
		// �ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		for (auto iterator = m_globs.begin(); iterator != m_globs.end(); ++iterator)
		{
			if (iterator->first == uri)
			{
				m_globs.erase(iterator);
				break;
			}
		}
		m_globs.push_back(make_pair(uri, servlet));
	}

	void ServletDispatch::addGlobServlet(const string &uri, function<int32_t(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
																			 shared_ptr<HttpSession> session)>
																callback)
	{
		return addGlobServlet(uri, shared_ptr<FunctionServlet>(new FunctionServlet(callback)));
	}

	void ServletDispatch::deleteServlet(const string &uri)
	{
		// �ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		m_datas.erase(uri);
	}

	void ServletDispatch::deleteGlobServlet(const string &uri)
	{
		// �ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		for (auto iterator = m_globs.begin(); iterator != m_globs.end(); ++iterator)
		{
			if (iterator->first == uri)
			{
				m_globs.erase(iterator);
				break;
			}
		}
	}

	shared_ptr<Servlet> ServletDispatch::getServlet(const string &uri)
	{
		// �ȼ��ӻ�����������
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
		auto iterator = m_datas.find(uri);
		return iterator == m_datas.end() ? nullptr : iterator->second;
	}

	shared_ptr<Servlet> ServletDispatch::getGlobServlet(const string &uri)
	{
		// �ȼ��ӻ�����������
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
		for (auto iterator = m_globs.begin(); iterator != m_globs.end(); ++iterator)
		{
			if (iterator->first == uri)
			{
				return iterator->second;
			}
		}
		return nullptr;
	}

	shared_ptr<Servlet> ServletDispatch::getMatched_servlet(const string &uri)
	{
		// �ȼ��ӻ�����������
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
		auto iterator = m_datas.find(uri);
		if (iterator != m_datas.end())
		{
			return iterator->second;
		}

		for (auto iterator = m_globs.begin(); iterator != m_globs.end(); ++iterator)
		{
			// ����ɹ�ƥ�䣨����0��
			if (fnmatch(iterator->first.c_str(), uri.c_str(), 0) == 0)
			{
				return iterator->second;
			}
		}
		return m_default_servlet;
	}

	int32_t ServletDispatch::handle(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
									shared_ptr<HttpSession> session)
	{
		auto servlet = getMatched_servlet(request->getPath());
		if (servlet)
		{
			servlet->handle(request, response, session);
		}
		return 0;
	}

	// class NotFoundServlet:public
	NotFoundServlet::NotFoundServlet()
		: Servlet("NotFoundServlet")
	{
		m_response_body = "<html><head><title>404 Not Found"
						  "</title></head><body><center><h1>404 Not Found</h1></center>"
						  "<hr><center>"
						  "sylar/1.0"
						  "</center></body></html>";
	}

	int32_t NotFoundServlet::handle(shared_ptr<HttpRequest> request, shared_ptr<HttpResponse> response,
									shared_ptr<HttpSession> session)
	{
		response->setStatus(HttpStatus::NOT_FOUND);
		response->setHeader("Server", "sylar/1.0.0");
		response->setHeader("Content-Type", "text/html");
		response->setBody(m_response_body);
		return 0;
	}
}
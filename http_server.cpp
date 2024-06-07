#include "http_server.h"

namespace HttpServerSpace
{
	using namespace HttpSessionSpace;
	using namespace LogSpace;
	using namespace SingletonSpace;
	using namespace UtilitySpace;

	//class HttpServer:public
	HttpServer::HttpServer(const bool is_keep_alive, IOManager* iomanager, IOManager* accept_iomanager)
		:TcpServer(iomanager, accept_iomanager), m_is_keep_alive(is_keep_alive)
	{
		m_servlet_dispatch.reset(new ServletDispatch());
	}
	
	//class HttpServer:protected
	void HttpServer::handleClient(shared_ptr<Socket> client_socket)
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "handleClient: " << client_socket;
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

		shared_ptr<HttpSession> session(new HttpSession(client_socket));
		do
		{
			auto request = session->receiveRequest();
			if (!request)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "receive http request fail,errno= " << errno
					<< " strerror=" << strerror(errno) << " client_socket=" << client_socket << " keep_alive=" << m_is_keep_alive;
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::WARN, log_event);
				break;
			}

			shared_ptr<HttpResponse> response(new HttpResponse(request->getVersion(), request->isClose() || !m_is_keep_alive));


			m_servlet_dispatch->handle(request, response, session);

			//response->setBody("hello world");
			session->sendResponse(response);

		} while (m_is_keep_alive);
		session->close();
	}
}
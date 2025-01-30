#pragma once
#include "tcp_ip/tcp_server.h"
#include "http/server/http_session.h"
#include "http/server/servlet.h"

namespace HttpServerSpace
{
	using namespace TcpServerSpace;
	using namespace ServletSpace;

	class HttpServer : public TcpServer
	{
	public:
		HttpServer(const bool is_keep_alive = false, IOManager *iomanager = IOManager::GetThis(), IOManager *accept_iomanager = IOManager::GetThis());

		shared_ptr<ServletDispatch> getServlet_dispatch() const { return m_servlet_dispatch; }
		void setServlet_dispatch(shared_ptr<ServletDispatch> servlet_dispatch) { m_servlet_dispatch = servlet_dispatch; }

	protected:
		virtual void handleClient(shared_ptr<Socket> client_socket) override;

	private:
		bool m_is_keep_alive;
		shared_ptr<ServletDispatch> m_servlet_dispatch;
	};
}
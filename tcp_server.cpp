#include "tcp_server.h"

namespace TcpServerSpace
{
	using namespace LogSpace;
	using namespace UtilitySpace;
	using namespace SingletonSpace;


	static const uint64_t t_tcp_server_read_timeout = 2 * 1000 * 60;

	//class TcpServer:public
	TcpServer::TcpServer(IOManager* iomanager, IOManager* accept_iomanager)
		:m_iomanager(iomanager),m_accept_iomanager(accept_iomanager), 
		m_receive_timeout(t_tcp_server_read_timeout), m_name("server/1.0.0"), m_is_stopped(true) {}

	TcpServer::~TcpServer()
	{
		for (auto& socket : m_sockets)
		{
			socket->close();	//???
		}
		m_sockets.clear();
	}

	bool TcpServer::bind(shared_ptr<Address> address)
	{
		vector<shared_ptr<Address>> addresses;	//???
		vector<shared_ptr<Address>> addresses_fail;
		addresses.push_back(address);
		return bind(addresses,addresses_fail);
	}
	bool TcpServer::bind(const vector<shared_ptr<Address>>& addresses, vector<shared_ptr<Address>>& addresses_fail)
	{
		bool return_value = true;
		for (auto& address : addresses)
		{
			shared_ptr<Socket> socket(new Socket((Socket::FamilyType)address->getFamily(), Socket::SocketType::TCP, 0));
			if (!socket->bind(address))
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "bind fail,errno=" << errno << " strerror=" << strerror(errno)
					<< " address=[" << address->toString() << "]";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
				return_value = false;
				addresses_fail.push_back(address);
				continue;
			}
			if (!socket->listen())
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "listen fail,errno=" << errno << " strerror=" << strerror(errno)
					<< " address=[" << address->toString() << "]";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
				return_value = false;
				addresses_fail.push_back(address);
				continue;
			}
			m_sockets.push_back(socket);
		}
		if (!addresses_fail.empty())
		{
			m_sockets.clear();	//???
			return false;
		}
		for (auto& socket : m_sockets)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "server bind success: " << socket;
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
		}
		return true;
	}
	
	void TcpServer::startAccept(shared_ptr<Socket> socket)
	{
		while (!isStopped())
		{
			shared_ptr<Socket> client_socket = socket->accept();
			if (client_socket!=nullptr)
			{
				client_socket->setReceive_timeout(m_receive_timeout);
				m_iomanager->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client_socket));
			}
			else
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "accept errno=" << errno << " strerror=" << strerror(errno);
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			}
		}
	}

	bool TcpServer::start()
	{
		if (!isStopped())
		{
			return true;
		}
		m_is_stopped = false;
		for (auto& socket : m_sockets)
		{
			m_accept_iomanager->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), socket));
		}
		return true;
	}
	
	void TcpServer::stop()
	{
		m_is_stopped = true;
		auto self_ptr = shared_from_this();
		m_accept_iomanager->schedule([this, self_ptr]()
			{
				for (auto& socket : m_sockets)
				{
					socket->settleAllEvents();
					socket->close();
				}
				m_sockets.clear();
			}
		);
	}

	//class TcpServer:protected
	void TcpServer::handleClient(shared_ptr<Socket> client_socket)
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		log_event->getSstream() << "handleClient: " << client_socket;
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
	}
}
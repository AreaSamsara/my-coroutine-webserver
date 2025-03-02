#include "tcp-ip/tcp-server.h"

namespace TcpServerSpace
{
	using namespace LogSpace;
	using namespace UtilitySpace;
	using namespace SingletonSpace;

	static const uint64_t t_tcp_server_read_timeout = 2 * 1000 * 60;

	// class TcpServer:public
	TcpServer::TcpServer(IOManager *iomanager, IOManager *accept_iomanager)
		: m_handle_iomanager(iomanager), m_accept_iomanager(accept_iomanager),
		  m_receive_timeout(t_tcp_server_read_timeout), m_name("server/1.0.0"), m_is_stopped(true) {}

	TcpServer::~TcpServer()
	{
		for (auto &socket : m_sockets)
		{
			socket->close(); //???
		}
		m_sockets.clear();
	}

	// 绑定并监听目标地址
	bool TcpServer::bind(shared_ptr<Address> local_address)
	{
		// 调用另一个版本的bind()方法
		vector<shared_ptr<Address>> remote_addresses;
		vector<shared_ptr<Address>> local_addresses_fail;
		remote_addresses.push_back(local_address);
		return bind(remote_addresses, local_addresses_fail);
	}

	// 绑定并监听列表中的地址
	bool TcpServer::bind(const vector<shared_ptr<Address>> &local_addresses, vector<shared_ptr<Address>> &local_addresses_fail)
	{
		bool return_value = true;
		for (auto &address : local_addresses)
		{
			// 创建服务器socket
			shared_ptr<Socket> socket(new Socket((Socket::FamilyType)address->getFamily(), Socket::SocketType::TCP, 0));

			// 绑定地址，绑定失败则在日志中报错并将当前地址加入失败地址列表
			if (!socket->bind(address))
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "bind fail,errno=" << errno << " strerror=" << strerror(errno)
										<< " address=[" << address->toString() << "]";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
				return_value = false;
				local_addresses_fail.push_back(address);
				continue;
			}

			// 监听地址，监听失败则在日志中报错并将当前地址加入失败地址列表
			if (!socket->listen())
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "listen fail,errno=" << errno << " strerror=" << strerror(errno)
										<< " address=[" << address->toString() << "]";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
				return_value = false;
				local_addresses_fail.push_back(address);
				continue;
			}

			m_sockets.push_back(socket);
		}

		// 如果至少有一个地址绑定或监听失败了，清空所有socket并返回false
		if (!local_addresses_fail.empty())
		{
			m_sockets.clear(); // 清除无效的socket
			return false;
		}

		// 分别为创建的所有socket打印日志
		for (auto &socket : m_sockets)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "server bind success: " << socket;
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
		}

		return true;
	}

	// 令当前socket开始接收请求
	void TcpServer::startAccept(shared_ptr<Socket> socket)
	{
		// 在服务器停止之前持续接收请求
		while (!isStopped())
		{
			shared_ptr<Socket> client_socket = socket->accept();

			// 如果接收到的客户端socket不为空，处理之
			if (client_socket != nullptr)
			{
				client_socket->setReceive_timeout(m_receive_timeout);
				m_handle_iomanager->schedule(std::bind(&TcpServer::handleClient, shared_from_this(), client_socket));
			}
			// 否则在日志中报错
			else
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "accept errno=" << errno << " strerror=" << strerror(errno);
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			}
		}
	}

	// 启动服务器
	bool TcpServer::start()
	{
		// 如果已经启动，直接返回true
		if (!isStopped())
		{
			return true;
		}

		// 修改停止状态
		m_is_stopped = false;

		// 为socket列表中的每一个socket都调度开始接受请求的方法
		for (auto &socket : m_sockets)
		{
			m_accept_iomanager->schedule(std::bind(&TcpServer::startAccept, shared_from_this(), socket));
		}

		return true;
	}

	// 停止服务器
	void TcpServer::stop()
	{
		// 修改停止状态
		m_is_stopped = true;

		auto self_ptr = shared_from_this();
		// ???暂时不知道为什么这样写
		m_accept_iomanager->schedule([this, self_ptr]()
									 {
				for (auto& socket : m_sockets)
				{
					socket->settleAllEvents();
					socket->close();
				}
				m_sockets.clear(); });
	}

	// class TcpServer:protected
	// 处理客户端请求（只打印日志，不作处理）
	void TcpServer::handleClient(shared_ptr<Socket> client_socket)
	{
		// 只打印日志，不作处理
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "handleClient: " << client_socket;
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
	}
}
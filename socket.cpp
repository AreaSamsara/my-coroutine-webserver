#include "socket.h"
#include <netinet/tcp.h>
#include "fdmanager.h"
#include "hook.h"


namespace SocketSpace
{
	using namespace FdManagerSpace;
	using namespace HookSpace;
	using std::dynamic_pointer_cast;

	//class Socket:public
	Socket::Socket(const FamilyType family, const SocketType type, const int protocol)
		:m_socket(-1),m_family(family),m_type(type),m_protocol(protocol),m_is_connected(false)
	{
		//如果要创建的是UDP socket，则直接创建socket文件描述符（否则在bind或connect中延迟创建）并默认处于已连接状态（实际上是无连接）
		if (type == UDP)
		{
			this->newSocket();
			m_is_connected = true;
		}
	}
	Socket::~Socket()
	{
		close();
	}

	//获取发送超时时间
	int64_t Socket::getSend_timout()const
	{
		//通过单例文件描述符管理者根据socket文件描述符获取对应的文件描述符实体
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(m_socket);
		//如果实体不为空，返回对应的发送超时时间，否则返回-1
		return file_descriptor_entity ? file_descriptor_entity->getTimeout(SO_SNDTIMEO) : -1;
	}
	//设置发送超时时间
	void Socket::setSend_timeout(const int64_t send_timeout)
	{
		timeval tv;
		tv.tv_sec = send_timeout / 1000;
		tv.tv_usec = send_timeout % 1000 * 1000;
		setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
	}

	//获取接收超时时间
	int64_t Socket::getReceive_timout()const
	{
		//通过单例文件描述符管理者根据socket文件描述符获取对应的文件描述符实体
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(m_socket);
		//如果实体不为空，返回对应的接收超时时间，否则返回-1
		return file_descriptor_entity ? file_descriptor_entity->getTimeout(SO_RCVTIMEO) : -1;
	}
	//设置接收超时时间
	void Socket::setReceive_timeout(const int64_t receive_timeout)
	{
		timeval tv;
		tv.tv_sec = receive_timeout / 1000;
		tv.tv_usec = receive_timeout % 1000 * 1000;
		setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
	}

	//获取socket选项
	bool Socket::getOption(const int level, const int option, void* result, socklen_t* len)
	{
		//获取socket选项，成功返回0
		return getsockopt(m_socket, level, option, result, (socklen_t*)len) == 0;
	}

	//设置socket选项
	bool Socket::setOption(const int level, const int option, const void * result, socklen_t len)
	{
		//设置socket选项，成功返回0
		return setsockopt(m_socket, level, option, result, len) == 0;
	}


	

	//绑定地址（在socket无效时创建新的socket文件描述符）
	bool Socket::bind(const shared_ptr<Address> address)
	{
		//如果socket无效，创建新的socket文件描述符
		if(isValid())
		{
			newSocket();
			//如果创建后socket仍为无效（小概率事件），返回false
			if (SYLAR_UNLIKELY(!isValid()))
			{
				return false;
			}
		}

		//如果socket协议族和要绑定的地址协议族不匹配(小概率事件)，报错并返回false
		if (SYLAR_UNLIKELY(address->getFamily() != m_family))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "bind family error,m_family=" << m_family << ") address_family("<<address->getFamily();
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		//调用全局bind()函数（成功则返回0），调用失败则报错并返回false
		if (::bind(m_socket, address->getAddress(), address->getAddress_length()) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "bind error,errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}
		
		//绑定成功后从系统读取本地地址并返回true
		getLocal_address();
		return true;
	}

	//监听socket
	bool Socket::listen(const int backlog = SOMAXCONN) const
	{
		//如果socket无效（小概率事件），报错并返回false
		if (SYLAR_UNLIKELY(!isValid()))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "listen error,socket invalid";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}
		//调用全局listen()函数（成功则返回0），调用失败则报错并返回false
		if (::listen(m_socket, backlog) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "listen error,errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}
		//否则返回true
		return true;
	}

	//接收connect链接
	shared_ptr<Socket> Socket::accept() const
	{
		//根据当前socket对象的信息创造一个同类的socket对象
		shared_ptr<Socket> socket(new Socket((FamilyType)m_family,(SocketType) m_type, m_protocol));

		//调用全局accept()函数接收链接（其hook版本会自动创建socket文件描述符对应的实体）
		int new_socket = ::accept(m_socket, nullptr, nullptr);
		//如果接收失败，报错并返回nullptr
		if (new_socket == -1)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "accept(" << m_socket << ") errno=" << errno << " strerr=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return nullptr;
		}
		else
		{
			//获取new_socket对应的文件描述符实体（应该已经在hook版本的accept()函数中创建）
			shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(new_socket);
			//如果该实体是socket且不处于关闭状态，则将socket对象初始化并返回
			if (file_descriptor_entity->isSocket() && !file_descriptor_entity->isClose())
			{
				socket->m_socket = new_socket;
				socket->m_is_connected = true;
				socket->initializeSocket();
				socket->getLocal_address();
				socket->getRemote_address();
				return socket;
			}
			//否则返回nullptr
			else
			{
				return nullptr;
			}
		}
	}


	//连接地址（在socket无效时创建新的socket文件描述符）
	bool Socket::connect(const shared_ptr<Address> address, const uint64_t timeout)
	{
		//如果socket无效，创建新的socket文件描述符
		if (!isValid())
		{
			newSocket();
			//如果创建后socket仍为无效（小概率事件），返回false
			if (SYLAR_UNLIKELY(!isValid()))
			{
				return false;
			}
		}

		//如果socket协议族和要绑定的地址协议族不匹配(小概率事件)，报错并返回false
		if (SYLAR_UNLIKELY(address->getFamily() != m_family))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "connect family error,m_family=" << m_family << ") address_family(" << address->getFamily();
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		//如果超时时间为-1，说明未设置超时时间
		if (timeout == (uint64_t)-1)
		{
			//调用全局connect()函数（成功返回0），调用失败则报错、关闭socket并返回false
			if (::connect(m_socket, address->getAddress(), address->getAddress_length()) != 0)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "connect error,errno=" << errno << " strerr = " << strerror(errno);
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
				close();
				return false;
			}
		}
		//否则说明设置了超时时间
		else
		{
			//调用全局connect_with_timeout()函数（成功返回0），调用失败则报错、关闭socket并返回false
			if (connect_with_timeout(m_socket, address->getAddress(), address->getAddress_length(), timeout))
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "connect_with_timeout error,errno=" << errno << " strerr = " << strerror(errno);
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
				close();
				return false;
			}
		}

		//连接成功后将socket设置为已连接
		m_is_connected = true;
		//从系统读取远端地址和本地地址并返回true
		getRemote_address();
		getLocal_address();
		return true;
	}


	//关闭socket
	//bool Socket::close()
	void Socket::close()
	{
		////如果不是已连接状态且socket文件描述符无效，说明已经关闭，直接返回true
		//if (!m_is_connected && m_socket == -1)
		//{
		//	return true;
		//}
		////否则设置为未连接状态，并在socket文件描述符有效时调用全局close()函数关闭之
		//else
		//{
		//	m_is_connected = false;
		//	if (m_socket != -1)
		//	{
		//		//调用全局的close()函数
		//		::close(m_socket);
		//		//将socket文件描述符设置为-1
		//		m_socket = -1;
		//	}
		//	return false;
		//}
		
		//如果是已连接状态，将其设置为未连接
		if (m_is_connected)
		{
			m_is_connected = false;
		}
		//如果socket文件描述符有效，关闭之
		if (isValid())
		{
			//调用全局的close()函数
			::close(m_socket);
			//将socket文件描述符设置为-1
			m_socket = -1;
		}
	}


	//发送数据
	int Socket::send(const void* buffer, const size_t length, const int flags) const
	{
		//如果当前处于已连接状态，调用全局send()函数并返回，否则返回-1
		return isConnected() ? ::send(m_socket, buffer, length, flags) : -1;
	}
	int Socket::send(const iovec* buffer, const size_t length, const int flags) const
	{
		//如果当前处于已连接状态，调用全局send()函数并返回
		if (isConnected())
		{
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec*)buffer;
			msg.msg_iovlen = length;
			return ::sendmsg(m_socket, &msg, flags);
		}
		//否则返回-1
		return -1;
	}
	int Socket::sendto(const void* buffer, const size_t length, const shared_ptr<Address> to, const int flags) const
	{
		//如果当前处于已连接状态，调用全局sendto()函数并返回，否则返回-1
		return isConnected() ? ::sendto(m_socket, buffer, length, flags, to->getAddress(), to->getAddress_length()) : -1;
	}
	int Socket::sendto(const iovec* buffer, const size_t length, const shared_ptr<Address> to, const int flags) const
	{
		//如果当前处于已连接状态，调用全局sendto()函数并返回
		if (isConnected())
		{
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec*)buffer;
			msg.msg_iovlen = length;
			msg.msg_name = to->getAddress();
			msg.msg_namelen = to->getAddress_length();
			return ::sendmsg(m_socket, &msg, flags);
		}
		//否则返回-1
		return -1;
	}

	//接收数据
	int Socket::recv(void* buffer, const size_t length, const int flags) const
	{
		//如果当前处于已连接状态，调用全局recv()函数并返回，否则返回-1
		return isConnected() ? ::recv(m_socket, buffer, length, flags) : -1;
	}
	int Socket::recv(iovec* buffer, const size_t length, const int flags) const
	{
		//如果当前处于已连接状态，调用全局recv()函数并返回
		if (isConnected())
		{
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec*)buffer;
			msg.msg_iovlen = length;
			return ::recvmsg(m_socket, &msg, flags);
		}
		//否则返回-1
		return -1;
	}
	int Socket::recvfrom(void* buffer, const size_t length, const shared_ptr<Address> from, const int flags) const
	{
		//如果当前处于已连接状态，调用全局recvfrom()函数并返回
		if (isConnected())
		{
			socklen_t address_length = from->getAddress_length();
			return ::recvfrom(m_socket, buffer, length, flags, from->getAddress(), &address_length);
		}
		//否则返回-1
		return -1;
	}
	int Socket::recvfrom(iovec* buffer, const size_t length, const shared_ptr<Address> from, const int flags) const
	{
		//如果当前处于已连接状态，调用全局recvfrom()函数并返回
		if (isConnected())
		{
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec*)buffer;
			msg.msg_iovlen = length;
			msg.msg_name = from->getAddress();
			msg.msg_namelen = from->getAddress_length();
			return ::recvmsg(m_socket, &msg, flags);
		}
		//否则返回-1
		return -1;
	}


	//返回socket是否有效
	bool Socket::isValid()const
	{
		return m_socket != -1;
	}
	//返回Socket错误
	int Socket::getError()
	{
		int error = 0;
		socklen_t length = sizeof(error);
		//调用getOption()函数尝试读取错误，成功则返回该错误，失败返回-1
		return getOption(SOL_SOCKET, SO_ERROR, &error, &length) ? error : -1;
	}


	//获取远端地址，并在首次调用时从系统读取之
	shared_ptr<Address> Socket::getRemote_address()
	{
		//如果远端地址已存在，直接返回之
		if (m_remote_address)
		{
			return m_remote_address;
		}

		shared_ptr<Address> remote_address;
		//根据socket的协议族为其分配对应类型的内存
		switch (m_family)
		{
		case AF_INET:
			remote_address.reset(new IPv4Address());
			break;
		case AF_INET6:
			remote_address.reset(new IPv6Address());
			break;
		case AF_UNIX:
			remote_address.reset(new UnixAddress());
			break;
		default:
			remote_address.reset(new UnknownAddress(m_family));
			break;
		}

		socklen_t length = remote_address->getAddress_length();

		//调用getsockname()函数获取远端地址信息（成功返回0），调用失败则报错并返回未知地址
		if (getpeername(m_socket, remote_address->getAddress(), &length) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "getpeername error,socket =" << m_socket << " errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return shared_ptr<Address>(new UnknownAddress(m_family));
		}

		//如果socket的协议族是Unix，还需要设置Unix地址长度
		if (m_family == AF_UNIX)
		{
			//检查智能指针转换是否安全
			shared_ptr<UnixAddress> address = dynamic_pointer_cast<UnixAddress>(remote_address);
			//设置Unix地址长度
			address->setAddress_length(length);
		}

		//根据获取到的结果设置本地地址并返回
		m_remote_address = remote_address;
		return m_remote_address;
	}

	//获取本地地址，并在首次调用时从系统读取之
	shared_ptr<Address> Socket::getLocal_address()
	{
		//如果本地地址已存在，直接返回之
		if (m_local_address)
		{
			return m_local_address;
		}

		shared_ptr<Address> local_address;
		//根据socket的协议族为其分配对应类型的内存
		switch (m_family)
		{
		case AF_INET:
			local_address.reset(new IPv4Address());
			break;
		case AF_INET6:
			local_address.reset(new IPv6Address());
			break;
		case AF_UNIX:
			local_address.reset(new UnixAddress());
			break;
		default:
			local_address.reset(new UnknownAddress(m_family));
			break;
		}

		socklen_t length = local_address->getAddress_length();

		//调用getsockname()函数获取本地地址信息（成功返回0），调用失败则报错并返回未知地址
		if (getsockname(m_socket, local_address->getAddress(), &length) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "getsockname error,socket =" << m_socket << " errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return shared_ptr<Address>(new UnknownAddress(m_family));
		}

		//如果socket的协议族是Unix，还需要设置Unix地址长度
		if (m_family == AF_UNIX)
		{
			//检查智能指针转换是否安全
			shared_ptr<UnixAddress> address = dynamic_pointer_cast<UnixAddress>(local_address);
			//设置Unix地址长度
			address->setAddress_length(length);
		}

		//根据获取到的结果设置本地地址并返回
		m_local_address = local_address;
		return m_local_address;
	}


	//输出信息到流中
	ostream& Socket::dump(ostream& os)const
	{
		os << "[Socket socket=" << m_socket
			<< " is_connected" << m_is_connected
			<< " family=" << m_family
			<< " type=" << m_type
			<< " protocol" << m_protocol;
		if (m_local_address)
		{
			os << " local_address=" << m_local_address->toString();
		}
		if (m_remote_address)
		{
			os << " remote_address=" << m_remote_address->toString();
		}
		os << "]";
		return os;
	}

	//结算读取事件
	bool Socket::settleRead_event()
	{
		return IOManager::GetThis()->settleEvent(m_socket, IOManager::READ);
	}
	//结算写入事件
	bool Socket::settleWrite_event()
	{
		return IOManager::GetThis()->settleEvent(m_socket, IOManager::WRITE);
	}
	//结算接收链接事件
	bool Socket::settleAccept_event()
	{
		return IOManager::GetThis()->settleEvent(m_socket, IOManager::READ);
	}
	//结算所有事件
	bool Socket::settleAllEvents()
	{
		return IOManager::GetThis()->settleAllEvents(m_socket);
	}


	//class Socket:public static
	//根据协议族、socket类型、协议创建Socket
	//shared_ptr<Socket> Socket::CreateSocket(const FamilyType family, const SocketType type, const int protocol)
	//{
	//	shared_ptr<Socket> socket(new Socket(family, type, protocol));
	//	//如果要创建的是UDP socket，则直接创建socket文件描述符（否则在bind或connect中延迟创建）并默认处于已连接状态（实际上是无连接）
	//	if (type == UDP)
	//	{
	//		socket->newSocket();
	//		socket->m_is_connected = true;
	//	}
	//	return socket;
	//}
			

	//class Socket:private
	//初始化socket文件描述符
	void Socket::initializeSocket()
	{
		int value = 1;
		setOption(SOL_SOCKET, SO_REUSEADDR, value);
		if (m_type == SOCK_STREAM)
		{
			//禁用Nagle算法，从而立即发送数据包而不是等待
			setOption(IPPROTO_TCP, TCP_NODELAY, value);
		}
	}
	//为Socket对象创建socket文件描述符（延迟初始化）
	void Socket::newSocket()
	{
		//调用原始的库函数创建socket
		m_socket = socket(m_family, m_type, m_protocol);
		//创建成功则将其初始化（大概率事件）
		if (SYLAR_LIKELY(m_socket != -1))
		{
			initializeSocket();
		}
		//否则报错
		else
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "socket(" << m_family << ", "<<m_type << ", "<< m_protocol 
				<< ") errno=" << errno << " strerr=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
		}
	}
}
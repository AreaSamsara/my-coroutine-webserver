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
	Socket::Socket(int family, int type, int protocol)
		:m_socket(-1),m_family(family),m_type(type),m_protocol(protocol),m_is_connected(false)
	{

	}
	Socket::~Socket()
	{
		close();
	}

	int64_t Socket::getSend_timout()const
	{
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(m_socket);
		if (file_descriptor_entity)
		{
			return file_descriptor_entity->getTimeout(SO_SNDTIMEO);
		}
		return -1;
	}
	void Socket::setSend_timeout(int64_t send_timeout)
	{
		timeval tv;
		tv.tv_sec = send_timeout / 1000;
		tv.tv_usec = send_timeout % 1000 * 1000;
		setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
	}

	int64_t Socket::getReceive_timout()const
	{
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(m_socket);
		if (file_descriptor_entity)
		{
			return file_descriptor_entity->getTimeout(SO_RCVTIMEO);
		}
		return -1;
	}
	void Socket::setReceive_timeout(int64_t receive_timeout)
	{
		timeval tv;
		tv.tv_sec = receive_timeout / 1000;
		tv.tv_usec = receive_timeout % 1000 * 1000;
		setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
	}

	bool Socket::getOption(int level, int option, void* result, socklen_t* len)
	{
		int return_value = getsockopt(m_socket, level, option, result, (socklen_t*)len);
		if (return_value)
		{
			return false;
		}
		return true;
	}


	bool Socket::setOption(int level, int option, const void * result, socklen_t len)
	{
		int return_value = setsockopt(m_socket, level, option, result, len);
		if (return_value)
		{
			return false;
		}
		return true;
	}


	shared_ptr<Socket> Socket::accept()
	{
		shared_ptr<Socket> socket(new Socket(m_family, m_type, m_protocol));
		int new_socket = ::accept(m_socket,nullptr,nullptr);
		if (new_socket == -1)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "accept(" << m_socket << ") errno=" << errno << " strerr=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return nullptr;
		}
		if (socket->init(new_socket))
		{
			return socket;
		}
		return nullptr;
	}

	bool Socket::init(int socket)
	{
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(socket);
		if (file_descriptor_entity->isSocket() && !file_descriptor_entity->isClose())
		{
			m_socket = socket;
			m_is_connected = true;
			initialize();
			getLocal_address();
			getRemote_address();
			return true;
		}
		return false;
	}

	bool Socket::bind(const shared_ptr<Address> address)
	{
		if(SYLAR_UNLIKELY(!isValid()))
		{
			newSocket();
			if (SYLAR_UNLIKELY(!isValid()))
			{
				return false;
			}
		}

		if (SYLAR_UNLIKELY(address->getFamily() != m_family))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "bind family error,m_family=" << m_family << ") address_family("<<address->getFamily();
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		if (::bind(m_socket, address->getAddress(), address->getAddress_length()))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "bind error,errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}
		
		getLocal_address();
		return true;
	}
	bool Socket::connect(const shared_ptr<Address> address, uint64_t timeout)
	{
		if (!isValid())
		{
			newSocket();
			if (SYLAR_UNLIKELY(!isValid()))
			{
				return false;
			}
		}

		if (SYLAR_UNLIKELY(address->getFamily() != m_family))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "connect family error,m_family=" << m_family << ") address_family(" << address->getFamily();
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		if (timeout == (uint64_t)-1)
		{
			if (::connect(m_socket, address->getAddress(), address->getAddress_length()))
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "connect error,errno=" << errno << " strerr = " << strerror(errno);
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
				close();
				return false;
			}
		}
		else
		{
			if (connect_with_timeout(m_socket,address->getAddress(),address->getAddress_length(),timeout))
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "connect_with_timeout error,errno=" << errno << " strerr = " << strerror(errno);
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
				close();
				return false;
			}
		}

		m_is_connected = true;
		getRemote_address();
		getLocal_address();
		return true;
	}
	bool Socket::listen(int backlog)
	{
		if (SYLAR_UNLIKELY(!isValid()))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "listen error,socket invalid";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
		}
		if (::listen(m_socket, backlog))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "listen error,errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}
		return true;
	}
	bool Socket::close()
	{
		if (!m_is_connected && m_socket == -1)
		{
			return true;
		}
		m_is_connected = false;
		if (m_socket != -1)
		{
			::close(m_socket);
			m_socket = -1;
		}
		return false;
	}

	int Socket::send(const void* buffer, size_t length, int flags)
	{
		if (isConnected())
		{
			return ::send(m_socket, buffer, length, flags);
		}
		return -1;
	}
	int Socket::send(const iovec* buffer, size_t length, int flags)
	{
		if (isConnected())
		{
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec*)buffer;
			msg.msg_iovlen = length;
			return ::sendmsg(m_socket, &msg, flags);
		}
		return -1;
	}
	int Socket::sendto(const void* buffer, size_t length, const shared_ptr<Address> to, int flags)
	{
		if (isConnected())
		{
			return ::sendto(m_socket, buffer, length, flags,to->getAddress(),to->getAddress_length());
		}
		return -1;
	}
	int Socket::sendto(const iovec* buffer, size_t length, const shared_ptr<Address> to, int flags)
	{
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
		return -1;
	}

	int Socket::recv(void* buffer, size_t length, int flags)
	{
		if (isConnected())
		{
			return ::recv(m_socket, buffer, length, flags);
		}
		return -1;
	}
	int Socket::recv(iovec* buffer, size_t length, int flags)
	{
		if (isConnected())
		{
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec*)buffer;
			msg.msg_iovlen = length;
			return ::recvmsg(m_socket, &msg, flags);
		}
		return -1;
	}
	int Socket::recvfrom(void* buffer, size_t length, shared_ptr<Address> from, int flags)
	{
		if (isConnected())
		{
			socklen_t length = from->getAddress_length();
			return ::recvfrom(m_socket, buffer, length, flags, from->getAddress(), &length);
		}
		return -1;
	}
	int Socket::recvfrom(iovec* buffer, size_t length, shared_ptr<Address> from, int flags)
	{
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
		return -1;
	}

	shared_ptr<Address> Socket::getRemote_address()
	{
		if (m_remote_address)
		{
			return m_remote_address;
		}
		shared_ptr<Address> result;
		switch (m_family)
		{
		case AF_INET:
			result.reset(new IPv4Address());
			break;
		case AF_INET6:
			result.reset(new IPv6Address());
			break;
		case AF_UNIX:
			result.reset(new UnixAddress());
			break;
		default:
			result.reset(new UnknowAddress(m_family));
			break;
		}

		socklen_t length = result->getAddress_length();
		if (getpeername(m_socket, result->getAddress(), &length))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "getpeername error,socket =" << m_socket << " errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return shared_ptr<Address>(new UnknowAddress(m_family));
		}
		if (m_family == AF_UNIX)
		{
			shared_ptr<UnixAddress> address = dynamic_pointer_cast<UnixAddress>(result);
			address->setAddress_length(length);
		}
		m_remote_address = result;
		return m_remote_address;
	}
	shared_ptr<Address> Socket::getLocal_address()
	{
		if (m_local_address)
		{
			return m_local_address;
		}
		shared_ptr<Address> result;
		switch (m_family)
		{
		case AF_INET:
			result.reset(new IPv4Address());
			break;
		case AF_INET6:
			result.reset(new IPv6Address());
			break;
		case AF_UNIX:
			result.reset(new UnixAddress());
			break;
		default:
			result.reset(new UnknowAddress(m_family));
			break;
		}

		socklen_t length = result->getAddress_length();
		if (getsockname(m_socket, result->getAddress(), &length))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "getsockname error,socket =" << m_socket << " errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return shared_ptr<Address>(new UnknowAddress(m_family));
		}
		if (m_family == AF_UNIX)
		{
			shared_ptr<UnixAddress> address = dynamic_pointer_cast<UnixAddress>(result);
			address->setAddress_length(length);
		}
		m_local_address = result;
		return m_local_address;
	}

	bool Socket::isValid()const
	{
		return m_socket != -1;
	}
	int Socket::getError()
	{
		int error = 0;
		socklen_t length = sizeof(error);
		if (!getOption(SOL_SOCKET, SO_ERROR, &error, &length))
		{
			return -1;
		}
		return error;
	}

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

	bool Socket::cancelRead()
	{
		return IOManager::GetThis()->settleEvent(m_socket, IOManager::READ);
	}
	bool Socket::cancelWrite()
	{
		return IOManager::GetThis()->settleEvent(m_socket, IOManager::WRITE);
	}
	bool Socket::cancelAccept()
	{
		return IOManager::GetThis()->settleEvent(m_socket, IOManager::READ);
	}
	bool Socket::cancelAll()
	{
		return IOManager::GetThis()->settleAllEvents(m_socket);
	}


	//class Socket:public static
	shared_ptr<Socket> Socket::CreateTCPSocket(shared_ptr<Address> address)
	{
		shared_ptr<Socket> socket(new Socket(address->getFamily(), TCP, 0));
		return socket;
	}
	shared_ptr<Socket> Socket::CreateTCPSocket()
	{
		shared_ptr<Socket> socket(new Socket(IPv4, TCP, 0));
		return socket;
	}
	shared_ptr<Socket> Socket::CreateUDPSocket(shared_ptr<Address> address)
	{
		shared_ptr<Socket> socket(new Socket(address->getFamily(), UDP, 0));
		return socket;
	}
	shared_ptr<Socket> Socket::CreateUDPSocket()
	{
		shared_ptr<Socket> socket(new Socket(IPv4, UDP, 0));
		return socket;
	}
	shared_ptr<Socket> Socket::CreateTCPSocket6(shared_ptr<Address> address)
	{
		shared_ptr<Socket> socket(new Socket(address->getFamily(), TCP, 0));
		return socket;
	}
	shared_ptr<Socket> Socket::CreateTCPSocket6()
	{
		shared_ptr<Socket> socket(new Socket(IPv6, TCP, 0));
		return socket;
	}
	shared_ptr<Socket> Socket::CreateUDPSocket6(shared_ptr<Address> address)
	{
		shared_ptr<Socket> socket(new Socket(address->getFamily(), UDP, 0));
		return socket;
	}
	shared_ptr<Socket> Socket::CreateUDPSocket6()
	{
		shared_ptr<Socket> socket(new Socket(IPv6, UDP, 0));
		return socket;
	}
	shared_ptr<Socket> Socket::CreateUnixSocket(shared_ptr<Address> address)
	{
		shared_ptr<Socket> socket(new Socket(address->getFamily(), TCP, 0));
		return socket;
	}
	shared_ptr<Socket> Socket::CreateUnixSocket()
	{
		shared_ptr<Socket> socket(new Socket(UNIX, TCP, 0));
		return socket;
	}
			

	//class Socket:private
	void Socket::initialize()
	{
		int value = 1;
		setOption(SOL_SOCKET, SO_REUSEADDR, value);
		if (m_type == SOCK_STREAM)
		{
			setOption(IPPROTO_TCP, TCP_NODELAY, value);
		}
	}
	void Socket::newSocket()
	{
		m_socket = socket(m_family, m_type, m_protocol);
		if (SYLAR_LIKELY(m_socket != -1))
		{
			initialize();
		}
		else
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "socket(" << m_family << ", "<<m_type << ", "<< m_protocol 
				<< ") errno=" << errno << " strerr=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
		}
	}
}
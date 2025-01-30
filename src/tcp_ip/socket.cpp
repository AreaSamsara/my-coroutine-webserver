#include "tcp_ip/socket.h"
#include <netinet/tcp.h>
#include "tcp_ip/fdmanager.h"
#include "concurrent/hook.h"

namespace SocketSpace
{
	using namespace FdManagerSpace;
	using namespace HookSpace;
	using std::dynamic_pointer_cast;

	// class Socket:public
	Socket::Socket(const FamilyType family, const SocketType type, const int protocol)
		: m_socket(-1), m_family(family), m_type(type), m_protocol(protocol), m_is_connected(false)
	{
		// ���Ҫ��������UDP socket����ֱ�Ӵ���socket�ļ���������������bind��connect���ӳٴ�������Ĭ�ϴ���������״̬��ʵ�����������ӣ�
		if (type == UDP)
		{
			this->newSocket();
			m_is_connected = true;
		}
	}
	// ����֮ǰ�ر�socket
	Socket::~Socket()
	{
		close();
	}

	// ��ȡ���ͳ�ʱʱ��
	int64_t Socket::getSend_timeout() const
	{
		// ͨ�������ļ������������߸���socket�ļ���������ȡ��Ӧ���ļ�������ʵ��
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(m_socket);
		// ���ʵ�岻Ϊ�գ����ض�Ӧ�ķ��ͳ�ʱʱ�䣬���򷵻�-1
		return file_descriptor_entity ? file_descriptor_entity->getTimeout(SO_SNDTIMEO) : -1;
	}
	// ���÷��ͳ�ʱʱ��
	void Socket::setSend_timeout(const int64_t send_timeout)
	{
		timeval tv;
		tv.tv_sec = send_timeout / 1000;
		tv.tv_usec = send_timeout % 1000 * 1000;
		setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
	}

	// ��ȡ���ճ�ʱʱ��
	int64_t Socket::getReceive_timeout() const
	{
		// ͨ�������ļ������������߸���socket�ļ���������ȡ��Ӧ���ļ�������ʵ��
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(m_socket);
		// ���ʵ�岻Ϊ�գ����ض�Ӧ�Ľ��ճ�ʱʱ�䣬���򷵻�-1
		return file_descriptor_entity ? file_descriptor_entity->getTimeout(SO_RCVTIMEO) : -1;
	}
	// ���ý��ճ�ʱʱ��
	void Socket::setReceive_timeout(const int64_t receive_timeout)
	{
		timeval tv;
		tv.tv_sec = receive_timeout / 1000;
		tv.tv_usec = receive_timeout % 1000 * 1000;
		setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
	}

	// ��ȡsocketѡ��
	bool Socket::getOption(const int level, const int option, void *result, socklen_t *len) const
	{
		// ��ȡsocketѡ��ɹ�����0
		return getsockopt(m_socket, level, option, result, (socklen_t *)len) == 0;
	}

	// ����socketѡ��
	bool Socket::setOption(const int level, const int option, const void *result, socklen_t len) const
	{
		// ����socketѡ��ɹ�����0
		return setsockopt(m_socket, level, option, result, len) == 0;
	}

	// �󶨵�ַ����socket��Чʱ�����µ�socket�ļ���������
	bool Socket::bind(const shared_ptr<Address> address)
	{
		// ���socket��Ч�������µ�socket�ļ�������
		if (!isValid())
		{
			newSocket();
			// ���������socket��Ϊ��Ч��С�����¼���������false
			if (SYLAR_UNLIKELY(!isValid()))
			{
				return false;
			}
		}

		// ���socketЭ�����Ҫ�󶨵ĵ�ַЭ���岻ƥ��(С�����¼�)������������false
		if (SYLAR_UNLIKELY(address->getFamily() != m_family))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "bind family error,m_family=" << m_family << ") address_family(" << address->getFamily();
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		// ����ȫ��bind()�������ɹ��򷵻�0��������ʧ���򱨴�������false
		if (::bind(m_socket, address->getAddress(), address->getAddress_length()) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "bind error,errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		// �󶨳ɹ����ϵͳ��ȡ���ص�ַ������true
		getLocal_address();
		return true;
	}

	// ����socket
	bool Socket::listen(const int backlog) const
	{
		// ���socket��Ч��С�����¼���������������false
		if (SYLAR_UNLIKELY(!isValid()))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "listen error,socket invalid";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}
		// ����ȫ��listen()�������ɹ��򷵻�0��������ʧ���򱨴�������false
		if (::listen(m_socket, backlog) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "listen error,errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}
		// ���򷵻�true
		return true;
	}

	// ����connect����
	shared_ptr<Socket> Socket::accept() const
	{
		// ���ݵ�ǰsocket�������Ϣ����һ��ͬ���socket����
		shared_ptr<Socket> socket(new Socket((FamilyType)m_family, (SocketType)m_type, m_protocol));

		// ����ȫ��accept()�����������ӣ���hook�汾���Զ�����socket�ļ���������Ӧ��ʵ�壩
		int new_socket = ::accept(m_socket, nullptr, nullptr);
		// �������ʧ�ܣ�����������nullptr
		if (new_socket == -1)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "accept(" << m_socket << ") errno=" << errno << " strerr=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return nullptr;
		}
		else
		{
			// ��ȡnew_socket��Ӧ���ļ�������ʵ�壨Ӧ���Ѿ���hook�汾��accept()�����д�����
			shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(new_socket);
			// �����ʵ����ڣ�����socket�������ڹر�״̬����socket�����ʼ��������
			if (file_descriptor_entity != nullptr && file_descriptor_entity->isSocket() && !file_descriptor_entity->isClose())
			{
				socket->m_socket = new_socket;
				socket->m_is_connected = true;
				socket->initializeSocket();
				socket->getLocal_address();
				socket->getRemote_address();
				return socket;
			}
			// ���򷵻�nullptr
			else
			{
				return nullptr;
			}
		}
	}

	// ���ӵ�ַ����socket��Чʱ�����µ�socket�ļ���������
	bool Socket::connect(const shared_ptr<Address> address, const uint64_t timeout)
	{
		// ���socket��Ч�������µ�socket�ļ�������
		if (!isValid())
		{
			newSocket();
			// ���������socket��Ϊ��Ч��С�����¼���������false
			if (SYLAR_UNLIKELY(!isValid()))
			{
				return false;
			}
		}

		// ���socketЭ�����Ҫ�󶨵ĵ�ַЭ���岻ƥ��(С�����¼�)������������false
		if (SYLAR_UNLIKELY(address->getFamily() != m_family))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "connect family error,m_family=" << m_family << ") address_family(" << address->getFamily();
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		// �����ʱʱ��Ϊ-1��˵��δ���ó�ʱʱ��
		if (timeout == (uint64_t)-1)
		{
			// ����ȫ��connect()�������ɹ�����0��������ʧ���򱨴����ر�socket������false
			if (::connect(m_socket, address->getAddress(), address->getAddress_length()) != 0)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "connect error,errno=" << errno << " strerr = " << strerror(errno);
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
				close();
				return false;
			}
		}
		// ����˵�������˳�ʱʱ��
		else
		{
			// ����ȫ��connect_with_timeout()�������ɹ�����0��������ʧ���򱨴����ر�socket������false
			if (connect_with_timeout(m_socket, address->getAddress(), address->getAddress_length(), timeout))
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "connect_with_timeout error,errno=" << errno << " strerr = " << strerror(errno);
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
				close();
				return false;
			}
		}

		// ���ӳɹ���socket����Ϊ������
		m_is_connected = true;
		// ��ϵͳ��ȡԶ�˵�ַ�ͱ��ص�ַ������true
		getRemote_address();
		getLocal_address();
		return true;
	}

	// �ر�socket
	void Socket::close()
	{
		// �����������״̬����������Ϊδ����
		if (m_is_connected)
		{
			m_is_connected = false;
		}
		// ���socket�ļ���������Ч���ر�֮
		if (isValid())
		{
			// ����ȫ�ֵ�close()����
			::close(m_socket);
			// ��socket�ļ�����������Ϊ-1
			m_socket = -1;
		}
	}

	// ��������
	int Socket::send(const void *buffer, const size_t length, const int flags) const
	{
		// �����ǰ����������״̬������ȫ��send()���������أ����򷵻�-1
		return isConnected() ? ::send(m_socket, buffer, length, flags) : -1;
	}
	int Socket::send(const iovec *buffer, const size_t length, const int flags) const
	{
		// �����ǰ����������״̬������ȫ��send()����������
		if (isConnected())
		{
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec *)buffer;
			msg.msg_iovlen = length;
			return ::sendmsg(m_socket, &msg, flags);
		}
		// ���򷵻�-1
		return -1;
	}
	int Socket::sendto(const void *buffer, const size_t length, const shared_ptr<Address> to, const int flags) const
	{
		// �����ǰ����������״̬������ȫ��sendto()���������أ����򷵻�-1
		return isConnected() ? ::sendto(m_socket, buffer, length, flags, to->getAddress(), to->getAddress_length()) : -1;
	}
	int Socket::sendto(const iovec *buffer, const size_t length, const shared_ptr<Address> to, const int flags) const
	{
		// �����ǰ����������״̬������ȫ��sendto()����������
		if (isConnected())
		{
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec *)buffer;
			msg.msg_iovlen = length;
			msg.msg_name = to->getAddress();
			msg.msg_namelen = to->getAddress_length();
			return ::sendmsg(m_socket, &msg, flags);
		}
		// ���򷵻�-1
		return -1;
	}

	// ��������
	int Socket::recv(void *buffer, const size_t length, const int flags) const
	{
		// �����ǰ����������״̬������ȫ��recv()���������أ����򷵻�-1
		return isConnected() ? ::recv(m_socket, buffer, length, flags) : -1;
	}
	int Socket::recv(iovec *buffer, const size_t length, const int flags) const
	{
		// �����ǰ����������״̬������ȫ��recv()����������
		if (isConnected())
		{
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec *)buffer;
			msg.msg_iovlen = length;
			return ::recvmsg(m_socket, &msg, flags);
		}
		// ���򷵻�-1
		return -1;
	}
	int Socket::recvfrom(void *buffer, const size_t length, const shared_ptr<Address> from, const int flags) const
	{
		// �����ǰ����������״̬������ȫ��recvfrom()����������
		if (isConnected())
		{
			socklen_t address_length = from->getAddress_length();
			return ::recvfrom(m_socket, buffer, length, flags, from->getAddress(), &address_length);
		}
		// ���򷵻�-1
		return -1;
	}
	int Socket::recvfrom(iovec *buffer, const size_t length, const shared_ptr<Address> from, const int flags) const
	{
		// �����ǰ����������״̬������ȫ��recvfrom()����������
		if (isConnected())
		{
			msghdr msg;
			memset(&msg, 0, sizeof(msg));
			msg.msg_iov = (iovec *)buffer;
			msg.msg_iovlen = length;
			msg.msg_name = from->getAddress();
			msg.msg_namelen = from->getAddress_length();
			return ::recvmsg(m_socket, &msg, flags);
		}
		// ���򷵻�-1
		return -1;
	}

	// ����socket�Ƿ���Ч
	bool Socket::isValid() const
	{
		return m_socket != -1;
	}
	// ����Socket����
	int Socket::getError() const
	{
		int error = 0;
		socklen_t length = sizeof(error);
		// ����getOption()�������Զ�ȡ���󣬳ɹ��򷵻ظô���ʧ�ܷ���-1
		return getOption(SOL_SOCKET, SO_ERROR, &error, &length) ? error : -1;
	}

	// ��ȡԶ�˵�ַ�������״ε���ʱ��ϵͳ��ȡ֮
	shared_ptr<Address> Socket::getRemote_address()
	{
		// ���Զ�˵�ַ�Ѵ��ڣ�ֱ�ӷ���֮
		if (m_remote_address)
		{
			return m_remote_address;
		}

		shared_ptr<Address> remote_address;
		// ����socket��Э����Ϊ������Ӧ���͵��ڴ�
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

		// ����getsockname()������ȡԶ�˵�ַ��Ϣ���ɹ�����0��������ʧ���򱨴�������δ֪��ַ
		if (getpeername(m_socket, remote_address->getAddress(), &length) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "getpeername error,socket =" << m_socket << " errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return shared_ptr<Address>(new UnknownAddress(m_family));
		}

		// ���socket��Э������Unix������Ҫ����Unix��ַ����
		if (m_family == AF_UNIX)
		{
			// �������ָ��ת���Ƿ�ȫ
			shared_ptr<UnixAddress> address = dynamic_pointer_cast<UnixAddress>(remote_address);
			// ����Unix��ַ����
			address->setAddress_length(length);
		}

		// ���ݻ�ȡ���Ľ�����ñ��ص�ַ������
		m_remote_address = remote_address;
		return m_remote_address;
	}

	// ��ȡ���ص�ַ�������״ε���ʱ��ϵͳ��ȡ֮
	shared_ptr<Address> Socket::getLocal_address()
	{
		// ������ص�ַ�Ѵ��ڣ�ֱ�ӷ���֮
		if (m_local_address)
		{
			return m_local_address;
		}

		shared_ptr<Address> local_address;
		// ����socket��Э����Ϊ������Ӧ���͵��ڴ�
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

		// ����getsockname()������ȡ���ص�ַ��Ϣ���ɹ�����0��������ʧ���򱨴�������δ֪��ַ
		if (getsockname(m_socket, local_address->getAddress(), &length) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "getsockname error,socket =" << m_socket << " errno=" << errno << " strerr = " << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return shared_ptr<Address>(new UnknownAddress(m_family));
		}

		// ���socket��Э������Unix������Ҫ����Unix��ַ����
		if (m_family == AF_UNIX)
		{
			// �������ָ��ת���Ƿ�ȫ
			shared_ptr<UnixAddress> address = dynamic_pointer_cast<UnixAddress>(local_address);
			// ����Unix��ַ����
			address->setAddress_length(length);
		}

		// ���ݻ�ȡ���Ľ�����ñ��ص�ַ������
		m_local_address = local_address;
		return m_local_address;
	}

	// �����Ϣ������
	ostream &Socket::dump(ostream &os) const
	{
		os << "[Socket socket=" << m_socket
		   << " is_connected=" << m_is_connected
		   << " family=" << m_family
		   << " type=" << m_type
		   << " protocol=" << m_protocol;
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

	// �����ȡ�¼�
	bool Socket::settleRead_event() const
	{
		return IOManager::GetThis()->settleEvent(m_socket, IOManager::READ);
	}
	// ����д���¼�
	bool Socket::settleWrite_event() const
	{
		return IOManager::GetThis()->settleEvent(m_socket, IOManager::WRITE);
	}
	// ������������¼�
	bool Socket::settleAccept_event() const
	{
		return IOManager::GetThis()->settleEvent(m_socket, IOManager::READ);
	}
	// ���������¼�
	bool Socket::settleAllEvents() const
	{
		return IOManager::GetThis()->settleAllEvents(m_socket);
	}

	// class Socket:public friend
	// ����<<����������ڽ���Ϣ���������
	ostream &operator<<(ostream &os, const shared_ptr<Socket> socket)
	{
		return socket->dump(os);
	}

	// class Socket:private
	// ��ʼ��socket�ļ�������
	void Socket::initializeSocket()
	{
		int value = 1;
		setOption(SOL_SOCKET, SO_REUSEADDR, value);
		if (m_type == SOCK_STREAM)
		{
			// ����Nagle�㷨���Ӷ������������ݰ������ǵȴ�
			setOption(IPPROTO_TCP, TCP_NODELAY, value);
		}
	}
	// ΪSocket���󴴽�socket�ļ����������ӳٳ�ʼ����
	void Socket::newSocket()
	{
		// ����ԭʼ�Ŀ⺯������socket
		m_socket = socket(m_family, m_type, m_protocol);
		// �����ɹ������ʼ����������¼���
		if (SYLAR_LIKELY(m_socket != -1))
		{
			initializeSocket();
		}
		// ���򱨴�
		else
		{
			// shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "socket(" << m_family << ", " << m_type << ", " << m_protocol
									<< ") errno=" << errno << " strerr=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
		}
	}
}
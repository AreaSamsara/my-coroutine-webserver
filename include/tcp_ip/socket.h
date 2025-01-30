#pragma once
#include <memory>

#include "tcp_ip/address.h"
#include "common/noncopyable.h"

namespace SocketSpace
{
	/*
	 * Socket����÷�����
	 * ����Э���塢Socket���͡�Э����Ϊ�������ù��캯������Socket����
	 * ��1������ǿͻ���Socket����ֱ�ӵ���connect()�������ӵ���Ӧ��ַ
	 * ��2������Ƿ����Socket����Ҫ����bind()�����󶨵���Ӧ��ַ���ٵ���listen()�������м�����������accept()�������ܿͻ���socket
	 * ����ٵ���send()ϵ�к�receive()ϵ�з������н���
	 */

	using namespace AddressSpace;
	using namespace NoncopyableSpace;

	// socket�࣬��ֹ����e
	class Socket : private Noncopyable
	{
	public:
		// ��ʾsocket���͵�ö������
		enum SocketType
		{
			TCP = SOCK_STREAM,
			UDP = SOCK_DGRAM
		};
		// ��ʾsocketЭ�����ö������
		enum FamilyType
		{
			IPv4 = AF_INET,
			IPv6 = AF_INET6,
			UNIX = AF_UNIX
		};

	public:
		Socket(const FamilyType family, const SocketType type, const int protocol = 0);
		// ����֮ǰ�ر�socket
		~Socket();

		// ��ȡ���ͳ�ʱʱ��
		int64_t getSend_timeout() const;
		// ���÷��ͳ�ʱʱ��
		void setSend_timeout(const int64_t send_timeout);

		// ��ȡ���ճ�ʱʱ��
		int64_t getReceive_timeout() const;
		// ���ý��ճ�ʱʱ��
		void setReceive_timeout(const int64_t receive_timeout);

		// ��ȡsocketѡ��
		bool getOption(const int level, const int option, void *result, socklen_t *len) const;
		template <class T>
		bool getOption(int level, int option, T &result)
		{
			socklen_t length = sizeof(T);
			return getOption(level, option, &result, &length);
		}

		// ����socketѡ��
		bool setOption(const int level, const int option, const void *result, socklen_t len) const;
		template <class T>
		bool setOption(int level, int option, const T &result)
		{
			return setOption(level, option, &result, sizeof(T));
		}

		// �󶨵�ַ����socket��Чʱ�����µ�socket�ļ���������
		bool bind(const shared_ptr<Address> address);
		// ����socket
		bool listen(const int backlog = SOMAXCONN) const;
		// ����connect����
		shared_ptr<Socket> accept() const;

		// ���ӵ�ַ����socket��Чʱ�����µ�socket�ļ���������
		bool connect(const shared_ptr<Address> address, const uint64_t timeout = -1);

		// �ر�socket
		void close();

		// ��������
		int send(const void *buffer, const size_t length, const int flags = 0) const;
		int send(const iovec *buffer, const size_t length, const int flags = 0) const;
		int sendto(const void *buffer, const size_t length, const shared_ptr<Address> to, const int flags = 0) const;
		int sendto(const iovec *buffer, const size_t length, const shared_ptr<Address> to, const int flags = 0) const;

		// ��������
		int recv(void *buffer, const size_t length, const int flags = 0) const;
		int recv(iovec *buffer, const size_t length, const int flags = 0) const;
		int recvfrom(void *buffer, const size_t length, const shared_ptr<Address> from, const int flags = 0) const;
		int recvfrom(iovec *buffer, const size_t length, const shared_ptr<Address> from, const int flags = 0) const;

		// ����˽�г�Ա
		int getSocket() const { return m_socket; }
		int getFamily() const { return m_family; }
		int getType() const { return m_type; }
		int getProtocol() const { return m_protocol; }
		bool isConnected() const { return m_is_connected; }

		// ����socket�Ƿ���Ч
		bool isValid() const;
		// ����Socket����
		int getError() const;

		// ��ȡԶ�˵�ַ�������״ε���ʱ��ϵͳ��ȡ֮
		shared_ptr<Address> getRemote_address();
		// ��ȡ���ص�ַ�������״ε���ʱ��ϵͳ��ȡ֮
		shared_ptr<Address> getLocal_address();

		// �����Ϣ������
		ostream &dump(ostream &os) const;

		// �����ȡ�¼�
		bool settleRead_event() const;
		// ����д���¼�
		bool settleWrite_event() const;
		// ������������¼�
		bool settleAccept_event() const;
		// ���������¼�
		bool settleAllEvents() const;

	public:
		// ����<<����������ڽ���Ϣ���������
		friend ostream &operator<<(ostream &os, const shared_ptr<Socket> socket);

	private:
		// ��ʼ��socket�ļ�������
		void initializeSocket();
		// ΪSocket���󴴽�socket�ļ����������ӳٳ�ʼ����
		void newSocket();

	private:
		// socket�ļ�������
		int m_socket;
		// Э���
		int m_family;
		// socket����
		int m_type;
		// Э��
		int m_protocol;
		// �Ƿ�������
		bool m_is_connected;

		// Զ�˵�ַ
		shared_ptr<Address> m_remote_address;
		// ���ص�ַ
		shared_ptr<Address> m_local_address;
	};
}
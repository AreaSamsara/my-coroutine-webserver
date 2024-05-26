#pragma once
#include <memory>

#include "address.h"
#include "noncopyable.h"

namespace SocketSpace
{
	using namespace AddressSpace;
	using namespace NoncopyableSpace;
	using std::enable_shared_from_this;

	//socket�࣬��ֹ����
	class Socket :public enable_shared_from_this<Socket>, private Noncopyable
	{
	public:
		//��ʾsocket���͵�ö������
		enum SocketType
		{
			TCP=SOCK_STREAM,
			UDP=SOCK_DGRAM
		};
		//��ʾsocketЭ�����ö������
		enum FamilyType
		{
			IPv4=AF_INET,
			IPv6=AF_INET6,
			UNIX=AF_UNIX
		};
	public:
		Socket(const FamilyType family, const SocketType type, const int protocol = 0);
		~Socket();

		//��ȡ���ͳ�ʱʱ��
		int64_t getSend_timout()const;
		//���÷��ͳ�ʱʱ��
		void setSend_timeout(const int64_t send_timeout);

		//��ȡ���ճ�ʱʱ��
		int64_t getReceive_timout()const;
		//���ý��ճ�ʱʱ��
		void setReceive_timeout(const int64_t receive_timeout);

		//��ȡsocketѡ��
		bool getOption(const int level, const int option, void* result, socklen_t* len);
		template<class T>
		bool getOption(int level, int option, T& result)
		{
			socklen_t length = sizeof(T);
			return getOption(level, option, &result, &length);
		}

		//����socketѡ��
		bool setOption(const int level, const int option, const void * result, socklen_t len);
		template<class T>
		bool setOption(int level, int option,const T& result)
		{
			return setOption(level, option, &result, sizeof(T));
		}

		

		//�󶨵�ַ����socket��Чʱ�����µ�socket�ļ���������
		bool bind(const shared_ptr<Address> address);
		//����socket
		bool listen(const int backlog) const;
		//����connect����
		shared_ptr<Socket> accept() const;

		//���ӵ�ַ����socket��Чʱ�����µ�socket�ļ���������
		bool connect(const shared_ptr<Address> address, const uint64_t timeout = -1);

		//�ر�socket
		//bool close();
		void close();

		//��������
		int send(const void* buffer, const size_t length, const int flags = 0) const;
		int send(const iovec* buffer, const size_t length, const int flags = 0) const;
		int sendto(const void* buffer, const size_t length, const shared_ptr<Address> to, const int flags = 0) const;
		int sendto(const iovec* buffer, const size_t length, const shared_ptr<Address> to, const int flags = 0) const;

		//��������
		int recv(void* buffer, const size_t length, const int flags = 0) const;
		int recv(iovec* buffer, const size_t length, const int flags = 0) const;
		int recvfrom(void* buffer, const size_t length, const shared_ptr<Address> from, const int flags = 0) const;
		int recvfrom(iovec* buffer, const size_t length, const shared_ptr<Address> from, const int flags = 0) const;

		
		//����˽�г�Ա
		int getSocket()const { return m_socket; }
		int getFamily()const { return m_family; }
		int getType()const { return m_type; }
		int getProtocol()const { return m_protocol; }
		bool isConnected()const { return m_is_connected; }

		//����socket�Ƿ���Ч
		bool isValid()const;
		//����Socket����
		int getError();

		//��ȡԶ�˵�ַ�������״ε���ʱ��ϵͳ��ȡ֮
		shared_ptr<Address> getRemote_address();
		//��ȡ���ص�ַ�������״ε���ʱ��ϵͳ��ȡ֮
		shared_ptr<Address> getLocal_address();

		//�����Ϣ������
		ostream& dump(ostream& os)const;
		
		//�����ȡ�¼�
		bool settleRead_event();
		//����д���¼�
		bool settleWrite_event();
		//������������¼�
		bool settleAccept_event();
		//���������¼�
		bool settleAllEvents();
	public:
		//����Э���塢socket���͡�Э�鴴��Socket
		//static shared_ptr<Socket> CreateSocket(const FamilyType family,const SocketType type,const int protocol = 0);
	private:
		//��ʼ��socket�ļ�������
		void initializeSocket();
		//ΪSocket���󴴽�socket�ļ����������ӳٳ�ʼ����
		void newSocket();
	private:
		//socket�ļ�������
		int m_socket;
		//Э���
		int m_family;
		//socket����
		int m_type;
		//Э��
		int m_protocol;
		//�Ƿ�������
		bool m_is_connected;

		//Զ�˵�ַ
		shared_ptr<Address> m_remote_address;
		//���ص�ַ
		shared_ptr<Address> m_local_address;
	};
}
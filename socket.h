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
		Socket(int family, int type, int protocol = 0);
		~Socket();

		//��ȡ���ͳ�ʱʱ��
		int64_t getSend_timout()const;
		//���÷��ͳ�ʱʱ��
		void setSend_timeout(int64_t send_timeout);

		//��ȡ���ճ�ʱʱ��
		int64_t getReceive_timout()const;
		//���ý��ճ�ʱʱ��
		void setReceive_timeout(int64_t receive_timeout);

		//��ȡsocketѡ��
		bool getOption(int level, int option, void* result, socklen_t* len);
		template<class T>
		bool getOption(int level, int option, T& result)
		{
			socklen_t length = sizeof(T);
			return getOption(level, option, &result, &length);
		}

		//����socketѡ��
		bool setOption(int level, int option, const void * result, socklen_t len);
		template<class T>
		bool setOption(int level, int option,const T& result)
		{
			return setOption(level, option, &result, sizeof(T));
		}

		//����connect����
		shared_ptr<Socket> accept();

		//�󶨵�ַ
		bool bind(const shared_ptr<Address> address);
		//���ӵ�ַ
		bool connect(const shared_ptr<Address> address, uint64_t timeout = -1);
		//����socket
		bool listen(int backlog = SOMAXCONN);
		//�ر�socket
		bool close();

		//��������
		int send(const void* buffer, size_t length, int flags = 0);
		int send(const iovec* buffer, size_t length, int flags = 0);
		int sendto(const void* buffer, size_t length, const shared_ptr<Address> to,int flags = 0);
		int sendto(const iovec* buffer, size_t length, const shared_ptr<Address> to, int flags = 0);

		//��������
		int recv(void* buffer, size_t length, int flags = 0);
		int recv(iovec* buffer, size_t length, int flags = 0);
		int recvfrom(void* buffer, size_t length, shared_ptr<Address> from, int flags = 0);
		int recvfrom(iovec* buffer, size_t length, shared_ptr<Address> from, int flags = 0);

		//��ȡԶ�˵�ַ�������״ε���ʱ��ϵͳ��ȡ֮
		shared_ptr<Address> getRemote_address();
		//��ȡ���ص�ַ�������״ε���ʱ��ϵͳ��ȡ֮
		shared_ptr<Address> getLocal_address();

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
		//����address��Э���崴��TCP���͵�Socket
		static shared_ptr<Socket> CreateTCPSocket(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateTCPSocket();
		//����address��Э���崴��UDP���͵�Socket
		static shared_ptr<Socket> CreateUDPSocket(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateUDPSocket();
		//����address��Э���崴��TCP���͵�Socket
		static shared_ptr<Socket> CreateTCPSocket6(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateTCPSocket6();
		//����address��Э���崴��UDP���͵�Socket
		static shared_ptr<Socket> CreateUDPSocket6(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateUDPSocket6();
		//����address��Э���崴��Unix���͵�Socket
		static shared_ptr<Socket> CreateUnixSocket(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateUnixSocket();

		//��ʼ��Socket����
		bool init(int socket);
	private:
		//��ʼ��socket�ļ�������
		void initialize();
		//����socket
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
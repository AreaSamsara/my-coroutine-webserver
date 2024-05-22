#pragma once
#include <memory>

#include "address.h"
#include "noncopyable.h"

namespace SocketSpace
{
	using namespace AddressSpace;
	using namespace NoncopyableSpace;
	using std::enable_shared_from_this;

	//socket类，禁止复制
	class Socket :public enable_shared_from_this<Socket>, private Noncopyable
	{
	public:
		//表示socket类型的枚举类型
		enum SocketType
		{
			TCP=SOCK_STREAM,
			UDP=SOCK_DGRAM
		};
		//表示socket协议族的枚举类型
		enum FamilyType
		{
			IPv4=AF_INET,
			IPv6=AF_INET6,
			UNIX=AF_UNIX
		};
	public:
		Socket(int family, int type, int protocol = 0);
		~Socket();

		//获取发送超时时间
		int64_t getSend_timout()const;
		//设置发送超时时间
		void setSend_timeout(int64_t send_timeout);

		//获取接收超时时间
		int64_t getReceive_timout()const;
		//设置接收超时时间
		void setReceive_timeout(int64_t receive_timeout);

		//获取socket选项
		bool getOption(int level, int option, void* result, socklen_t* len);
		template<class T>
		bool getOption(int level, int option, T& result)
		{
			socklen_t length = sizeof(T);
			return getOption(level, option, &result, &length);
		}

		//设置socket选项
		bool setOption(int level, int option, const void * result, socklen_t len);
		template<class T>
		bool setOption(int level, int option,const T& result)
		{
			return setOption(level, option, &result, sizeof(T));
		}

		//接收connect链接
		shared_ptr<Socket> accept();

		//绑定地址
		bool bind(const shared_ptr<Address> address);
		//连接地址
		bool connect(const shared_ptr<Address> address, uint64_t timeout = -1);
		//监听socket
		bool listen(int backlog = SOMAXCONN);
		//关闭socket
		bool close();

		//发送数据
		int send(const void* buffer, size_t length, int flags = 0);
		int send(const iovec* buffer, size_t length, int flags = 0);
		int sendto(const void* buffer, size_t length, const shared_ptr<Address> to,int flags = 0);
		int sendto(const iovec* buffer, size_t length, const shared_ptr<Address> to, int flags = 0);

		//接收数据
		int recv(void* buffer, size_t length, int flags = 0);
		int recv(iovec* buffer, size_t length, int flags = 0);
		int recvfrom(void* buffer, size_t length, shared_ptr<Address> from, int flags = 0);
		int recvfrom(iovec* buffer, size_t length, shared_ptr<Address> from, int flags = 0);

		//获取远端地址，并在首次调用时从系统读取之
		shared_ptr<Address> getRemote_address();
		//获取本地地址，并在首次调用时从系统读取之
		shared_ptr<Address> getLocal_address();

		//返回私有成员
		int getSocket()const { return m_socket; }
		int getFamily()const { return m_family; }
		int getType()const { return m_type; }
		int getProtocol()const { return m_protocol; }
		bool isConnected()const { return m_is_connected; }

		//返回socket是否有效
		bool isValid()const;
		//返回Socket错误
		int getError();

		//输出信息到流中
		ostream& dump(ostream& os)const;
		
		//结算读取事件
		bool settleRead_event();
		//结算写入事件
		bool settleWrite_event();
		//结算接收链接事件
		bool settleAccept_event();
		//结算所有事件
		bool settleAllEvents();
	public:
		//根据address的协议族创建TCP类型的Socket
		static shared_ptr<Socket> CreateTCPSocket(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateTCPSocket();
		//根据address的协议族创建UDP类型的Socket
		static shared_ptr<Socket> CreateUDPSocket(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateUDPSocket();
		//根据address的协议族创建TCP类型的Socket
		static shared_ptr<Socket> CreateTCPSocket6(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateTCPSocket6();
		//根据address的协议族创建UDP类型的Socket
		static shared_ptr<Socket> CreateUDPSocket6(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateUDPSocket6();
		//根据address的协议族创建Unix类型的Socket
		static shared_ptr<Socket> CreateUnixSocket(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateUnixSocket();

		//初始化Socket对象
		bool init(int socket);
	private:
		//初始化socket文件描述符
		void initialize();
		//创建socket
		void newSocket();
	private:
		//socket文件描述符
		int m_socket;
		//协议簇
		int m_family;
		//socket类型
		int m_type;
		//协议
		int m_protocol;
		//是否已连接
		bool m_is_connected;

		//远端地址
		shared_ptr<Address> m_remote_address;
		//本地地址
		shared_ptr<Address> m_local_address;
	};
}
#pragma once
#include <memory>

#include "address.h"
#include "noncopyable.h"

namespace SocketSpace
{
	/*
	* Socket类调用方法：
	* 先用协议族、Socket类型、协议作为参数调用构造函数创建Socket对象，
	* （1）如果是客户端Socket，则直接调用connect()方法连接到对应地址
	* （2）如果是服务端Socket，需要先用bind()方法绑定到对应地址，再调用listen()方法进行监听，并调用accept()方法接受客户端socket
	* 最后再调用send()系列和receive()系列方法进行交互
	*/

	using namespace AddressSpace;
	using namespace NoncopyableSpace;

	//socket类，禁止复制e
	class Socket : private Noncopyable
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
		Socket(const FamilyType family, const SocketType type, const int protocol = 0);
		//析构之前关闭socket
		~Socket();

		//获取发送超时时间
		int64_t getSend_timeout()const;
		//设置发送超时时间
		void setSend_timeout(const int64_t send_timeout);

		//获取接收超时时间
		int64_t getReceive_timeout()const;
		//设置接收超时时间
		void setReceive_timeout(const int64_t receive_timeout);

		//获取socket选项
		bool getOption(const int level, const int option, void* result, socklen_t* len) const;
		template<class T>
		bool getOption(int level, int option, T& result)
		{
			socklen_t length = sizeof(T);
			return getOption(level, option, &result, &length);
		}

		//设置socket选项
		bool setOption(const int level, const int option, const void * result, socklen_t len) const;
		template<class T>
		bool setOption(int level, int option,const T& result)
		{
			return setOption(level, option, &result, sizeof(T));
		}

		

		//绑定地址（在socket无效时创建新的socket文件描述符）
		bool bind(const shared_ptr<Address> address);
		//监听socket
		bool listen(const int backlog = SOMAXCONN)const;
		//接收connect链接
		shared_ptr<Socket> accept()const;

		//连接地址（在socket无效时创建新的socket文件描述符）
		bool connect(const shared_ptr<Address> address, const uint64_t timeout = -1);

		//关闭socket
		void close();

		//发送数据
		int send(const void* buffer, const size_t length, const int flags = 0) const;
		int send(const iovec* buffer, const size_t length, const int flags = 0) const;
		int sendto(const void* buffer, const size_t length, const shared_ptr<Address> to, const int flags = 0) const;
		int sendto(const iovec* buffer, const size_t length, const shared_ptr<Address> to, const int flags = 0) const;

		//接收数据
		int recv(void* buffer, const size_t length, const int flags = 0) const;
		int recv(iovec* buffer, const size_t length, const int flags = 0) const;
		int recvfrom(void* buffer, const size_t length, const shared_ptr<Address> from, const int flags = 0) const;
		int recvfrom(iovec* buffer, const size_t length, const shared_ptr<Address> from, const int flags = 0) const;

		
		//返回私有成员
		int getSocket()const { return m_socket; }
		int getFamily()const { return m_family; }
		int getType()const { return m_type; }
		int getProtocol()const { return m_protocol; }
		bool isConnected()const { return m_is_connected; }

		//返回socket是否有效
		bool isValid()const;
		//返回Socket错误
		int getError()const;

		//获取远端地址，并在首次调用时从系统读取之
		shared_ptr<Address> getRemote_address();
		//获取本地地址，并在首次调用时从系统读取之
		shared_ptr<Address> getLocal_address();

		//输出信息到流中
		ostream& dump(ostream& os)const;
		
		//结算读取事件
		bool settleRead_event() const;
		//结算写入事件
		bool settleWrite_event() const;
		//结算接收链接事件
		bool settleAccept_event() const;
		//结算所有事件
		bool settleAllEvents() const;
	public:
		//重载<<运算符，用于将信息输出到流中
		friend ostream& operator<<(ostream& os, const shared_ptr<Socket> socket);
	private:
		//初始化socket文件描述符
		void initializeSocket();
		//为Socket对象创建socket文件描述符（延迟初始化）
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
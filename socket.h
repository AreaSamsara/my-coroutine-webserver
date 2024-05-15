#pragma once
#include <memory>

#include "address.h"
#include "noncopyable.h"

namespace SocketSpace
{
	using namespace AddressSpace;
	using namespace NoncopyableSpace;
	using std::enable_shared_from_this;

	class Socket :public enable_shared_from_this<Socket>, private Noncopyable
	{
	public:
		enum SocketType
		{
			TCP=SOCK_STREAM,
			UDP=SOCK_DGRAM
		};
		enum FamilyType
		{
			IPv4=AF_INET,
			IPv6=AF_INET6,
			UNIX=AF_UNIX
		};
	public:
		Socket(int family, int type, int protocol = 0);
		~Socket();

		int64_t getSend_timout()const;
		void setSend_timeout(int64_t send_timeout);

		int64_t getReceive_timout()const;
		void setReceive_timeout(int64_t receive_timeout);

		bool getOption(int level, int option, void* result, socklen_t* len);
		template<class T>
		bool getOption(int level, int option, T& result)
		{
			socklen_t length = sizeof(T);
			return getOption(level, option, &result, &length);
		}

		bool setOption(int level, int option, const void * result, socklen_t len);
		template<class T>
		bool setOption(int level, int option,const T& result)
		{
			return setOption(level, option, &result, sizeof(T));
		}

		shared_ptr<Socket> accept();

		bool init(int socket);

		bool bind(const shared_ptr<Address> address);
		bool connect(const shared_ptr<Address> address, uint64_t timeout = -1);
		bool listen(int backlog = SOMAXCONN);
		bool close();

		int send(const void* buffer, size_t length, int flags = 0);
		int send(const iovec* buffer, size_t length, int flags = 0);
		int sendto(const void* buffer, size_t length, const shared_ptr<Address> to,int flags = 0);
		int sendto(const iovec* buffer, size_t length, const shared_ptr<Address> to, int flags = 0);

		int recv(void* buffer, size_t length, int flags = 0);
		int recv(iovec* buffer, size_t length, int flags = 0);
		int recvfrom(void* buffer, size_t length, shared_ptr<Address> from, int flags = 0);
		int recvfrom(iovec* buffer, size_t length, shared_ptr<Address> from, int flags = 0);

		shared_ptr<Address> getRemote_address();
		shared_ptr<Address> getLocal_address();

		int getSocket()const { return m_socket; }
		int getFamily()const { return m_family; }
		int getType()const { return m_type; }
		int getProtocol()const { return m_protocol; }

		bool isConnected()const { return m_is_connected; }
		bool isValid()const;
		int getError();

		ostream& dump(ostream& os)const;
		
		bool cancelRead();
		bool cancelWrite();
		bool cancelAccept();
		bool cancelAll();
	public:
		static shared_ptr<Socket> CreateTCPSocket(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateTCPSocket();
		static shared_ptr<Socket> CreateUDPSocket(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateUDPSocket();
		static shared_ptr<Socket> CreateTCPSocket6(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateTCPSocket6();
		static shared_ptr<Socket> CreateUDPSocket6(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateUDPSocket6();
		static shared_ptr<Socket> CreateUnixSocket(shared_ptr<Address> address);
		static shared_ptr<Socket> CreateUnixSocket();
	private:
		void initialize();
		void newSocket();
	private:
		int m_socket;
		int m_family;
		int m_type;
		int m_protocol;
		bool m_is_connected;

		shared_ptr<Address> m_remote_address;
		shared_ptr<Address> m_local_address;
	};
}
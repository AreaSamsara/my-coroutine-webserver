#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <memory>
#include <vector>
#include <map>

namespace AddressSpace
{
	using std::ostream;
	using std::string;
	using std::multimap;
	using std::shared_ptr;
	using std::vector;
	using std::pair;

	class Address;
	class IPAddress;

	class Address
	{
	public:
		virtual ~Address() {}

		virtual const sockaddr* getAddress()const = 0;
		virtual sockaddr* getAddress() = 0;
		virtual socklen_t getAddress_length()const = 0;
		virtual	ostream& insert(ostream& os)const = 0;

		int getFamily()const;
		string toString();

		bool operator<(const Address& rhs)const;
		bool operator==(const Address& rhs)const;
		bool operator!=(const Address & rhs)const;
	public:
		static shared_ptr<Address> Create(const sockaddr* address, socklen_t addrlen);
		//默认返回IPv4的地址，因为其通用且速度快于IPv6
		static bool Lookup(vector<shared_ptr<Address>>& result, const string& host,
			int family = AF_INET, int type = 0, int protocol = 0);
		static shared_ptr<Address> LookupAny(const string& host, int family = AF_INET,
			int type = 0, int protocol = 0);
		static shared_ptr<IPAddress> LookupAnyIPAddress(const string& host, int family = AF_INET,
			int type = 0, int protocol = 0);
		static bool GetInterfaceAddresses(multimap<string, pair<shared_ptr<Address>, uint32_t>>& result,
			int family = AF_INET);
		static bool GetInterfaceAddresses(vector<pair<shared_ptr<Address>,uint32_t>>& result,
			const string& iface,int family = AF_INET);
	};

	class IPAddress :public Address
	{
	public:
		virtual shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) = 0;
		virtual shared_ptr<IPAddress> networdAddress(uint32_t prefix_len) = 0;
		virtual shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) = 0;

		virtual uint16_t getPort()const = 0;
		virtual void setPort(uint16_t port) = 0;
	public:
		static shared_ptr<IPAddress> Create(const char* address, uint32_t port = 0);
	};

	class IPv4Address :public IPAddress
	{
	public:
		IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
		IPv4Address(const sockaddr_in& address);

		virtual const sockaddr* getAddress()const override;
		virtual sockaddr* getAddress()override;
		virtual socklen_t getAddress_length()const override;
		virtual	ostream& insert(ostream& os)const override;

		virtual shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) override;
		virtual shared_ptr<IPAddress> networdAddress(uint32_t prefix_len) override;
		virtual shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) override;
		virtual uint16_t getPort()const override;
		virtual void setPort(uint16_t port) override;
	public:
		static shared_ptr<IPv4Address> Create(const char* address, uint16_t port);
	private:
		sockaddr_in m_address;
	};

	class IPv6Address : public IPAddress
	{
	public:
		IPv6Address();
		IPv6Address(const uint8_t address[16], uint16_t port = 0);
		IPv6Address(const sockaddr_in6& address);

		virtual const sockaddr* getAddress()const override;
		virtual sockaddr* getAddress() override;
		virtual socklen_t getAddress_length()const override;
		virtual	ostream& insert(ostream& os)const override;

		virtual shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) override;
		virtual shared_ptr<IPAddress> networdAddress(uint32_t prefix_len) override;
		virtual shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) override;
		virtual uint16_t getPort()const override;
		virtual void setPort(uint16_t port) override;
	public:
		static shared_ptr<IPv6Address> Create(const char* address, uint16_t port);
	private:
		sockaddr_in6 m_address;
	};

	class UnixAddress : public Address
	{
	public:
		UnixAddress();
		UnixAddress(const string& path);

		virtual const sockaddr* getAddress()const override;
		virtual sockaddr* getAddress() override;
		virtual socklen_t getAddress_length()const override;
		void setAddress_length(uint32_t address_length);
		virtual	ostream& insert(ostream& os)const override;
	private:
		sockaddr_un m_address;
		socklen_t m_length;
	};

	class UnknowAddress :public Address
	{
	public:
		UnknowAddress(int family);
		UnknowAddress(const sockaddr& address);

		virtual const sockaddr* getAddress()const override;
		virtual sockaddr* getAddress() override;
		virtual socklen_t getAddress_length()const override;
		virtual	ostream& insert(ostream& os)const override;
	private:
		sockaddr m_address;
	};
}
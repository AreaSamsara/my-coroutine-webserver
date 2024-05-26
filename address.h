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

	//class Address;
	class IPAddress;

	//地址类（抽象基类）
	class Address
	{
	public:
		virtual ~Address() {}

		//获取只读版地址指针
		virtual const sockaddr* getAddress()const = 0;
		//获取地址指针
		virtual sockaddr* getAddress() = 0;
		//获取地址的长度
		virtual socklen_t getAddress_length()const = 0;
		//获取可读性输出地址
		virtual	ostream& getAddress_ostream(ostream& os)const = 0;

		//返回协议族
		int getFamily()const { return getAddress()->sa_family; }
		//将地址转化为字符串
		string toString();

		//重载比较运算符
		bool operator<(const Address& right_address)const;
		bool operator==(const Address& right_address)const;
		bool operator!=(const Address& right_address)const;
	public:
		//用sockaddr指针创建Address对象
		static shared_ptr<Address> CreateAddress(const sockaddr* address, socklen_t addrlen);
		//通过host地址返回符合条件的所有Address(放入传入的容器中)
		static bool Lookup(vector<shared_ptr<Address>>& addresses, const string& host,
			int family = AF_INET, int type = 0, int protocol = 0);		//默认返回IPv4的地址，因为其通用且速度快于IPv6
		//通过host地址返回符合条件的任一Address
		static shared_ptr<Address> LookupAny(const string& host, int family = AF_INET,
			int type = 0, int protocol = 0);
		//通过host地址返回符合条件的任一IPAddress
		static shared_ptr<IPAddress> LookupAnyIPAddress(const string& host, int family = AF_INET,
			int type = 0, int protocol = 0);
		//返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
		static bool GetInterfaceAddresses(multimap<string, pair<shared_ptr<Address>, uint32_t>>& result,
			int family = AF_INET);
		//获取指定网卡的地址和子网掩码位数
		static bool GetInterfaceAddresses(vector<pair<shared_ptr<Address>, uint32_t>>& result,
			const string& iface, int family = AF_INET);
	private:
		//计算参数二进制表达中1的位数
		template<class T>
		static uint32_t CountBytes(T value)
		{
			uint32_t result = 0;
			for (; value; ++result)
			{
				value &= value - 1;
			}
			return result;
		}
	};

	//IP地址类（基类），公有继承自Address类
	class IPAddress :public Address
	{
	public:
		//获取该地址的广播地址
		virtual shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) = 0;
		//获取该地址的网段
		virtual shared_ptr<IPAddress> networdAddress(uint32_t prefix_len) = 0;
		//获取子网掩码地址
		virtual shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) = 0;

		//获取端口号
		virtual uint16_t getPort()const = 0;
		//设置端口号
		virtual void setPort(uint16_t port) = 0;
	public:
		//通过域名,IP,服务器名创建IPAddress
		static shared_ptr<IPAddress> CreateIPAddress(const char* address, uint32_t port = 0);
	protected:
		//生成掩码（最低的bits位为1）
		template<class T>
		static T CreateMask(uint32_t bits)
		{
			return (1 << (sizeof(T) * 8 - bits)) - 1;
		}
	};

	//IPv4地址类，公有继承自IPAddress类
	class IPv4Address :public IPAddress
	{
	public:
		//通过IPv4二进制地址构造IPv4Address
		IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);
		//通过sockaddr_in构造IPv4Address
		IPv4Address(const sockaddr_in& address) { m_address = address; }
		//使用点分十进制地址构造IPv4Address
		IPv4Address(const char* address, uint16_t port);	//new

		//获取只读版地址指针
		virtual const sockaddr* getAddress()const override { return (sockaddr*)&m_address; }
		//获取地址指针
		virtual sockaddr* getAddress()override { return (sockaddr*)&m_address; }
		//获取地址的长度
		virtual socklen_t getAddress_length()const override { return sizeof(m_address); }
		//获取可读性输出地址
		virtual	ostream& getAddress_ostream(ostream& os)const override;

		//获取该地址的广播地址
		virtual shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) override;
		//获取该地址的网段
		virtual shared_ptr<IPAddress> networdAddress(uint32_t prefix_len) override;
		//获取子网掩码地址
		virtual shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) override;

		//获取端口号
		virtual uint16_t getPort()const override;
		//设置端口号
		virtual void setPort(uint16_t port) override;
	private:
		//IPv4地址结构体
		sockaddr_in m_address;
	};

	//IPv6地址类，公有继承自IPAddress类
	class IPv6Address : public IPAddress
	{
	public:
		IPv6Address();
		//通过IPv6二进制地址构造IPv6Address
		IPv6Address(const uint8_t address[16], uint16_t port = 0);
		//通过sockaddr_in6构造IPv6Address
		IPv6Address(const sockaddr_in6& address) { m_address = address; }
		//通过IPv6地址字符串构造IPv6Address
		IPv6Address(const char* address, uint16_t port);	//new

		//获取只读版地址指针
		virtual const sockaddr* getAddress()const override { return (sockaddr*)&m_address; }
		//获取地址指针
		virtual sockaddr* getAddress() override { return (sockaddr*)&m_address; }
		//获取地址的长度
		virtual socklen_t getAddress_length()const override { return sizeof(m_address); }
		//获取可读性输出地址
		virtual	ostream& getAddress_ostream(ostream& os)const override;

		//获取该地址的广播地址
		virtual shared_ptr<IPAddress> broadcastAddress(uint32_t prefix_len) override;
		//获取该地址的网段
		virtual shared_ptr<IPAddress> networdAddress(uint32_t prefix_len) override;
		//获取子网掩码地址
		virtual shared_ptr<IPAddress> subnetMask(uint32_t prefix_len) override;

		//获取端口号
		virtual uint16_t getPort()const override;
		//设置端口号
		virtual void setPort(uint16_t port) override;
	private:
		//IPv6地址结构体
		sockaddr_in6 m_address;
	};

	//Unix地址类，公有继承自Address类
	class UnixAddress : public Address
	{
	public:
		UnixAddress();
		//通过路径字符串构造UnixAddress
		UnixAddress(const string& path);

		//获取只读版地址指针
		virtual const sockaddr* getAddress()const override { return (sockaddr*)&m_address; }
		//获取地址指针
		virtual sockaddr* getAddress() override { return (sockaddr*)&m_address; }
		//获取地址的长度
		virtual socklen_t getAddress_length()const override { return m_length; }
		//获取可读性输出地址
		virtual	ostream& getAddress_ostream(ostream& os)const override;

		//设置地址长度
		void setAddress_length(uint32_t address_length) { m_length = address_length; }
	private:
		//Unix地址结构体
		sockaddr_un m_address;
		//Unix地址长度
		socklen_t m_length;
	};

	//未知地址类，公有继承自Address类
	class UnknownAddress :public Address
	{
	public:
		UnknownAddress(int family);
		//通过sockaddr构造UnknownAddress
		UnknownAddress(const sockaddr& address) { m_address = address; }

		//获取只读版地址指针
		virtual const sockaddr* getAddress()const override { return &m_address; }
		//获取地址指针
		virtual sockaddr* getAddress() override { return &m_address; }
		//获取地址的长度
		virtual socklen_t getAddress_length()const override { return sizeof(m_address); }
		//获取可读性输出地址
		virtual	ostream& getAddress_ostream(ostream& os)const override;
	private:
		//通用地址结构体
		sockaddr m_address;
	};
}
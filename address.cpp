#include "address.h"
#include <sstream>
#include <cstring>
#include <cstddef>
#include <netdb.h>
#include <ifaddrs.h>

#include "endian.h"
#include "log.h"
#include "utility.h"
#include "singleton.h"

namespace AddressSpace
{
	using namespace LogSpace;
	using namespace UtilitySpace;
	using namespace SingletonSpace;

	using std::stringstream;
	using std::min;
	using std::hex;
	using std::dec;
	using std::logic_error;
	using std::dynamic_pointer_cast;


	template<class T>
	//生成子网掩码的反码（最低的bits位为1）
	static T CreateMask(uint32_t bits)
	{
		return (1 << (sizeof(T) * 8 - bits)) - 1;
	}

	template<class T>
	//计算参数二进制表达中1的位数
	static uint32_t CountBytes(T value)
	{
		uint32_t result = 0;
		for (; value; ++result)
		{
			value &= value - 1;
		}
		return result;
	}

	//class Address:public
	//返回协议族
	int Address::getFamily()const
	{
		return getAddress()->sa_family;
	}
	//将地址转化为字符串
	string Address::toString()
	{
		stringstream ss;
		getAddress_ostream(ss);
		return ss.str();
	}

	bool Address::operator<(const Address& rhs)const
	{
		//取二者的较小长度作为memcmp的长度
		socklen_t minlength = min(getAddress_length(), rhs.getAddress_length());
		//先比较内存中数据的大小
		int result = memcmp(getAddress(), rhs.getAddress(), minlength);
		if (result < 0)
		{
			return true;
		}
		else if (result > 0)
		{
			return false;
		}
		//若内存中数据的大小相等，再比较地址长度
		else if (getAddress_length() < rhs.getAddress_length())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	bool Address::operator==(const Address& rhs)const
	{
		//当且仅当内存中数据的大小相等且地址长度相等时返回true
		return getAddress_length() == rhs.getAddress_length()
			&& memcmp(getAddress(), rhs.getAddress(), getAddress_length()) == 0;
	}
	bool Address::operator!=(const Address& rhs)const
	{
		return !(*this == rhs);
	}


	//class Address:public static
	//用sockaddr指针创建Address对象
	shared_ptr<Address> Address::Create(const sockaddr* address, socklen_t addrlen)
	{
		//如果sockaddr指针为空，直接返回nullptr
		if (address == nullptr)
		{
			return nullptr;
		}

		shared_ptr<Address> result;
		//根据address的协议族调用对应的构造函数创建Address对象
		switch (address->sa_family)
		{
		case AF_INET:
			result.reset(new IPv4Address(*(const sockaddr_in*)address));
			break;
		case AF_INET6:
			result.reset(new IPv6Address(*(const sockaddr_in6*)address));
			break;
		default:
			result.reset(new UnknownAddress(*address));
			break;
		}
		return result;
	}

	//通过host地址返回符合条件的所有Address(放入传入的容器中)
	bool Address::Lookup(vector<shared_ptr<Address>>& addresses, const string& host, int family, int type, int protocol)
	{
		//设置搜索条件
		addrinfo hints;
		hints.ai_flags = 0;
		hints.ai_family = family;
		hints.ai_socktype = type;
		hints.ai_protocol = protocol;
		hints.ai_addrlen = 0;
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;

		//搜索结果
		addrinfo* results = NULL;

		//地址字符串（不包括中括号）
		string address_str;
		//端口号字符指针
		const char* port_char_ptr = NULL;

		//如果host地址不为空且以'['开头，说明其为携带中括号的IPv6地址，检查该ipv6地址是否携带端口号
		if (!host.empty() && host[0] == '[')
		{
			//查找host地址字符串中']'第一次出现的位置，即为IPv6地址（不包含端口号）的末尾
			const char* end_ipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
			//如果查找成功
			if (end_ipv6)
			{
				//如果地址末尾的下一个字符为':'说明该host地址携带了端口号，读取之
				if (*(end_ipv6 + 1) == ':')
				{
					port_char_ptr = end_ipv6 + 2;
				}
				//将地址设置为中括号以内的部分
				address_str = host.substr(1, end_ipv6 - host.c_str() - 1);
			}
		}

		//如果地址字符串为空，说明其不携带中括号
		if (address_str.empty())
		{
			//查找host地址字符串中':'第一次出现的位置，即为端口号字符指针的前一个位置
			port_char_ptr = (const char*)memchr(host.c_str(), ':', host.size());
			//如果查找成功，说明该host地址包含端口号
			if (port_char_ptr)
			{
				//如果在接下来的部分中不再有':'，说明可以设置地址和端口号字符指针（？？？）
				if (!memchr(port_char_ptr + 1, ':', host.c_str() + host.size() - port_char_ptr - 1))
				{
					//将地址设置为':'之前的部分
					address_str = host.substr(0, port_char_ptr - host.c_str());
					//将端口号字符指针移动一位，使其指向正确的位置
					++port_char_ptr;
				}
			}
		}

		//如果地址字符串为空，说明其不携带端口号，直接将其设为host地址（此时端口号字符指针为默认值NULL，getaddrinfo()函数的搜索将不考虑端口号）
		if (address_str.empty())
		{
			address_str = host;
		}

		//调用getaddrinfo()函数尝试解析address的信息（成功返回0）
		int error = getaddrinfo(address_str.c_str(), port_char_ptr, &hints, &results);
		//如果解析失败，报错并返回false
		if (error != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "Address::Lookup getaddrinfo(" << host << ", " << family << ", " << type
				<< ") error=" << error << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		//用搜索结果创建Address对象，并依次放入addresses中
		addrinfo* next_result = results;
		while (next_result)
		{
			addresses.push_back(Create(next_result->ai_addr, (socklen_t)next_result->ai_addrlen));
			next_result = next_result->ai_next;
		}

		//清理地址信息结构体
		freeaddrinfo(results);
		return true;
	}

	//通过host地址返回符合条件的任一Address
	shared_ptr<Address> Address::LookupAny(const string& host, int family,int type, int protocol)
	{
		vector<shared_ptr<Address>> addresses;
		//调用Lookup()方法，如果成功则返回获取的第一个Address
		if (Lookup(addresses, host, family, type, protocol))
		{
			return addresses[0];
		}
		//否则返回nullptr
		return nullptr;
	}

	//通过host地址返回符合条件的任一IPAddress
	shared_ptr<IPAddress> Address::LookupAnyIPAddress(const string& host, int family ,int type, int protocol)
	{
		vector<shared_ptr<Address>> addresses;
		//调用Lookup()方法，如果成功则返回获取的第一个能够成功通过智能指针转换转换成IPAddress的Address
		if (Lookup(addresses, host, family, type, protocol))
		{
			for (auto& address : addresses)
			{
				shared_ptr<IPAddress> ip_address = dynamic_pointer_cast<IPAddress>(address);
				if (ip_address)
				{
					return ip_address;
				}
			}
		}
		//否则返回nullptr
		return nullptr;
	}
	
	//返回本机所有网卡的<网卡名, 地址, 子网掩码位数>
	bool Address::GetInterfaceAddresses(multimap<string, pair<shared_ptr<Address>, uint32_t>>& result, int family)
	{
		ifaddrs* next=NULL,*results=NULL;
		if (getifaddrs(&results) != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "Address::GetInterfaceAddresses getifaddrs" << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			return false;
		}

		try
		{
			for (next = results; next; next = next->ifa_next)
			{
				shared_ptr<Address> address;
				uint32_t prefix_length = ~0u;
				if (family != AF_UNSPEC && family != next->ifa_addr->sa_family)
				{
					continue;
				}
				switch (next->ifa_addr->sa_family)
				{
				case AF_INET:
				{
					address = Create(next->ifa_addr, sizeof(sockaddr_in));
					uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
					prefix_length = CountBytes(netmask);
				}
					break;
				case AF_INET6:
				{
					address = Create(next->ifa_addr, sizeof(sockaddr_in6));
					in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
					prefix_length = 0;
					for (int i = 0; i < 16; ++i)
					{
						prefix_length += CountBytes(netmask.s6_addr[i]);
					}
				}
					break;
				default:
					break;
				}

				if (address)
				{
					result.insert(make_pair(next->ifa_name, make_pair(address, prefix_length)));
				}
			}
		}
		catch (...)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "Address::GetInterfaceAddresses exception" << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);

			freeifaddrs(results);
			return false;
		}

		freeifaddrs(results);
		return true;
	}

	//获取指定网卡的地址和子网掩码位数
	bool Address::GetInterfaceAddresses(vector<pair<shared_ptr<Address>,uint32_t>>& result, const string& iface, int family)
	{
		if (iface.empty() || iface == "*")
		{
			if (family == AF_INET || family == AF_UNSPEC)
			{
				result.push_back(make_pair(shared_ptr<Address>(new IPv4Address()), 0u));
			}
			if (family == AF_INET6 || family == AF_UNSPEC)
			{
				result.push_back(make_pair(shared_ptr<Address>(new IPv6Address()), 0u));
			}
			return true;
		}

		multimap<string, pair<shared_ptr<Address>, uint32_t>> results;
		if (!GetInterfaceAddresses(results, family))
		{
			return false;
		}

		auto iterators = results.equal_range(iface);
		for (;iterators.first!=iterators.second;++iterators.first)
		{
			result.push_back(iterators.first->second);
		}
		return true;
	}


	//class IPAddress :public static
	//通过域名,IP,服务器名创建IPAddress
	shared_ptr<IPAddress> IPAddress::Create(const char* address, uint32_t port)
	{
		//设置搜索条件
		addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		// 获取规范主机名
		//hints.ai_flags = AI_NUMERICHOST;
		hints.ai_flags = AI_CANONNAME;	//根据弹幕建议修改的
		// 指定地址族为未指定（让getaddrinfo决定）
		hints.ai_family = AF_UNSPEC;

		//搜索结果
		addrinfo* results = NULL;

		//调用getaddrinfo()函数尝试解析address的信息（成功返回0）
		int error = getaddrinfo(address, NULL, &hints, &results);
		//如果解析失败，报错并返回nullptr
		if (error != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "IPAddress::Create(" << address << ", " << port
				<< ") errno=" << errno << " strerror=" << strerror(error);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			return nullptr;
		}

		//尝试用取得的信息创建IPAddress对象
		try
		{
			//保证智能指针转换的安全
			shared_ptr<IPAddress> ip_address = dynamic_pointer_cast<IPAddress>(Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
			//如果创建成功则设置其端口
			if (ip_address)
			{
				ip_address->setPort(port);
			}
			//清理搜索结果并返回创建的IPAddress对象
			freeaddrinfo(results);
			return ip_address;
		}
		//如果智能指针转换出错，清理搜索结果并返回nullptr
		catch (...)
		{
			freeaddrinfo(results);
			return nullptr;
		}
	}

	//class IPv4Address :public
	//通过IPv4二进制地址构造IPv4Address
	IPv4Address::IPv4Address(uint32_t address, uint16_t port)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sin_family = AF_INET;
		m_address.sin_port = htons(port);
		m_address.sin_addr.s_addr = htonl(address);
	}
	//通过sockaddr_in构造IPv4Address
	IPv4Address::IPv4Address(const sockaddr_in& address)
	{
		m_address = address;
	}

	const sockaddr* IPv4Address::getAddress()const
	{
		return (sockaddr*)&m_address;
	}
	sockaddr* IPv4Address::getAddress()
	{
		return (sockaddr*)&m_address;
	}
	socklen_t IPv4Address::getAddress_length()const
	{
		return sizeof(m_address);
	}

	ostream& IPv4Address::getAddress_ostream(ostream& os)const
	{
		uint32_t address = ntohl(m_address.sin_addr.s_addr);
		//将IPv4的点分十进制地址写入流中（&0xff用于保留最低的8位）
		os << ((address >> 24) & 0xff) << "."
			<< ((address >> 16) & 0xff) << "."
			<< ((address >> 8) & 0xff) << "."
			<< (address & 0xff);
		//将端口号写入流中
		os << ":" << ntohs(m_address.sin_port);
		return os;
	}

	shared_ptr<IPAddress> IPv4Address::broadcastAddress(uint32_t prefix_len)
	{
		//子网掩码的掩盖位数不应该超过32位，否则返回nullptr
		if (prefix_len > 32)
		{
			return nullptr;
		}

		//将当前地址与子网掩码的反码取或，得到广播地址
		sockaddr_in broadcast_address(m_address);
		broadcast_address.sin_addr.s_addr |= htonl(CreateMask<uint32_t>(prefix_len));

		return shared_ptr<IPv4Address>(new IPv4Address(broadcast_address));
	}

	shared_ptr<IPAddress> IPv4Address::networdAddress(uint32_t prefix_len)
	{
		//子网掩码的掩盖位数不应该超过32位，否则返回nullptr
		if (prefix_len > 32)
		{
			return nullptr;
		}

		//将当前地址与子网掩码的反码取与，得到网段（此处有误）
		sockaddr_in broadcast_address(m_address);
		broadcast_address.sin_addr.s_addr &= htonl(CreateMask<uint32_t>(prefix_len));

		return shared_ptr<IPv4Address>(new IPv4Address(broadcast_address));
	}

	shared_ptr<IPAddress> IPv4Address::subnetMask(uint32_t prefix_len)
	{
		//对子网掩码对象只设置协议族，其他不必要的成员置空，提高效率
		sockaddr_in subnet_mask;
		memset(&subnet_mask, 0, sizeof(subnet_mask));
		subnet_mask.sin_family = AF_INET;
		//对子网掩码的反码取反，得到子网掩码（？？？）
		subnet_mask.sin_addr.s_addr = ~htonl(CreateMask<uint32_t>(prefix_len));
		return shared_ptr<IPv4Address>(new IPv4Address(subnet_mask));
	}

	uint16_t IPv4Address::getPort()const
	{
		return ntohs(m_address.sin_port);
	}
	void IPv4Address::setPort(uint16_t port)
	{
		m_address.sin_port = htons(port);
	}


	//class IPv4Address :public static
	//使用点分十进制地址创建IPv4Address
	shared_ptr<IPv4Address> IPv4Address::Create(const char* address, uint16_t port)
	{
		shared_ptr<IPv4Address> ipv4_address(new IPv4Address);
		ipv4_address->m_address.sin_port = htons(port);
		int addr = inet_pton(AF_INET, address, &ipv4_address->m_address.sin_addr);
		//如果inet_pton()调用失败，报错并返回nullptr
		if (addr <= 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "IPv4Address::Create(" << address << ", " << port << ") return=" << addr
				<< " errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			return nullptr;
		}
		return ipv4_address;
	}



	//class IPv6Address : public
	IPv6Address::IPv6Address()
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sin6_family = AF_INET6;
	}
	//通过IPv6二进制地址构造IPv6Address
	IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sin6_family = AF_INET6;
		m_address.sin6_port = htons(port);
		memcpy(&m_address.sin6_addr.s6_addr, address, 16);
	}
	//通过sockaddr_in6构造IPv6Address
	IPv6Address::IPv6Address(const sockaddr_in6& address)
	{
		m_address = address;
	}

	const sockaddr* IPv6Address::getAddress()const
	{
		return (sockaddr*)&m_address;
	}
	sockaddr* IPv6Address::getAddress()
	{
		return (sockaddr*)&m_address;
	}
	socklen_t IPv6Address::getAddress_length()const
	{
		return sizeof(m_address);
	}
	ostream& IPv6Address::getAddress_ostream(ostream& os)const
	{
		os << "[";

		//以16位（每两个冒号间的4个16进制数占16位）为步长处理IPv6地址（s6_addr数组存储是从地址的高位开始的）
		uint16_t* address = (uint16_t*)m_address.sin6_addr.s6_addr;
		//是否已经使用过了零压缩（每串IPv6地址中只允许使用一次以避免歧义）
		bool used_zeros = false;

		//依次处理每一段地址
		for (size_t i = 0; i < 8; ++i)
		{
			//跳过首次遇到的所有连续的零，直到被非零段中断为止
			if (address[i] == 0 && !used_zeros)
			{
				continue;
			}
			//在首次遇到的连续的零被中断后，使用零压缩（多输出一个冒号以形成双冒号）
			if (i > 0 && address[i - 1] == 0 && !used_zeros)
			{
				os << ":";
				used_zeros = true;
			}
			//如果处理的不是第一段16位，先输出冒号
			if (i > 0)
			{
				os << ":";
			}
			//按16进制将该段地址输出到流
			os << hex << (int)ntohs(address[i]) << dec;
		}
		
		//如果首次遇到的连续的零在末尾，则直接在末尾输出双冒号
		if (!used_zeros && address[7] == 0)
		{
			os << "::";
		}

		os << "]:" << ntohs(m_address.sin6_port);
		return os;
	}

	shared_ptr<IPAddress> IPv6Address::broadcastAddress(uint32_t prefix_len)
	{
		//将当前地址与子网掩码的反码取或，得到广播地址
		sockaddr_in6 broadcast_address(m_address);
		//只对临界的字节进行或操作
		broadcast_address.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
		//临界字节以后的字节直接置高（s6_addr数组存储是从地址的高位开始的）
		for (size_t i = prefix_len / 8 + 1; i < 16; ++i)
		{
			broadcast_address.sin6_addr.s6_addr[i] = 0xff;
		}

		return shared_ptr<IPv6Address>(new IPv6Address(broadcast_address));
	}
	shared_ptr<IPAddress> IPv6Address::networdAddress(uint32_t prefix_len)
	{
		//将当前地址与子网掩码的反码取与，得到网段（此处有误）
		sockaddr_in6 netword_address(m_address);
		//只对临界的字节进行与操作
		netword_address.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
		//临界字节以后的字节直接置低（s6_addr数组存储是从地址的高位开始的）
		for (size_t i = prefix_len / 8 + 1; i < 16; ++i)
		{
			netword_address.sin6_addr.s6_addr[i] = 0x00;
		}

		return shared_ptr<IPv6Address>(new IPv6Address(netword_address));
	}
	shared_ptr<IPAddress> IPv6Address::subnetMask(uint32_t prefix_len)
	{
		//对子网掩码的反码取反，得到子网掩码（？？？）
		sockaddr_in6 subnet_mask;
		//对子网掩码对象只设置协议族，其他不必要的成员置空，提高效率
		memset(&subnet_mask, 0, sizeof(subnet_mask));
		subnet_mask.sin6_family = AF_INET6;
		//只对临界的字节进行取反操作
		subnet_mask.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);
		//临界字节以前的字节直接置高（s6_addr数组存储是从地址的高位开始的）
		for (size_t i = 0; i < prefix_len / 8; ++i)
		{
			subnet_mask.sin6_addr.s6_addr[i] = 0xff;
		}

		return shared_ptr<IPv6Address>(new IPv6Address(subnet_mask));
	}
	uint16_t IPv6Address::getPort()const
	{
		return ntohs(m_address.sin6_port);
	}
	void IPv6Address::setPort(uint16_t port)
	{
		m_address.sin6_port = htons(port);
	}

	//class IPv6Address :public static
	//通过IPv6地址字符串构造IPv6Address
	shared_ptr<IPv6Address> IPv6Address::Create(const char* address, uint16_t port)
	{
		shared_ptr<IPv6Address> ipv6_address(new IPv6Address);
		ipv6_address->m_address.sin6_port = htons(port);
		int addr = inet_pton(AF_INET6, address, &ipv6_address->m_address.sin6_addr);
		//如果inet_pton()调用失败，报错并返回nullptr
		if (addr <= 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "IPv6Address::Create(" << address << ", " << port << ") return=" << addr
				<< " errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			return nullptr;
		}
		return ipv6_address;
	}

	//class UinxAddress :public
	static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;
	UnixAddress::UnixAddress()
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sun_family = AF_UNIX;
		m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
	}

	//通过路径字符串构造UnixAddress
	UnixAddress::UnixAddress(const string& path)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sun_family = AF_UNIX;
		m_length = path.size() + 1;

		if (!path.empty() && path[0] == '\0')
		{
			--m_length;
		}
		if (m_length > sizeof(m_address.sun_path))
		{
			throw logic_error("path too long");
		}
		memcpy(m_address.sun_path, path.c_str(), m_length);
		m_length += offsetof(sockaddr_un, sun_path);
	}

	const sockaddr* UnixAddress::getAddress()const
	{
		return (sockaddr*)&m_address;
	}
	sockaddr* UnixAddress::getAddress()
	{
		return (sockaddr*)&m_address;
	}
	socklen_t UnixAddress::getAddress_length()const
	{
		return m_length;
	}
	void UnixAddress::setAddress_length(uint32_t address_length)
	{
		m_length = address_length;
	}
	ostream& UnixAddress::getAddress_ostream(ostream& os)const
	{
		if (m_length > offsetof(sockaddr_un, sun_path) && m_address.sun_path[0]=='\0')
		{
			return os << "\\0" << string(m_address.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
		}
		return os << m_address.sun_path;
	}


	//class UnknowAddress :public
	UnknownAddress::UnknownAddress(int family)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sa_family = family;
	}
	//通过sockaddr构造UnknownAddress
	UnknownAddress::UnknownAddress(const sockaddr& address)
	{
		m_address = address;
	}

	const sockaddr* UnknownAddress::getAddress()const
	{
		return &m_address;
	}
	sockaddr* UnknownAddress::getAddress()
	{
		return &m_address;
	}
	socklen_t UnknownAddress::getAddress_length()const
	{
		return sizeof(m_address);
	}
	ostream& UnknownAddress::getAddress_ostream(ostream& os)const
	{
		os << "[Unknow Address,family=" << m_address.sa_family << "]";
		return os;
	}
}
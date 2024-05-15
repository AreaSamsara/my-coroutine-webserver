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
	//using namespace EndianSpace;
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
	static T CreateMask(uint32_t bits)
	{
		return (1 << (sizeof(T) * 8 - bits)) - 1;
	}

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

	//class Address:public
	int Address::getFamily()const
	{
		return getAddress()->sa_family;
	}
	string Address::toString()
	{
		stringstream ss;
		insert(ss);
		return ss.str();
	}

	bool Address::operator<(const Address& rhs)const
	{
		socklen_t minlength = min(getAddress_length(), rhs.getAddress_length());
		int result = memcmp(getAddress(), rhs.getAddress(), minlength);
		if (result < 0)
		{
			return true;
		}
		else if (result > 0)
		{
			return false;
		}
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
		return getAddress_length() == rhs.getAddress_length()
			&& memcmp(getAddress(), rhs.getAddress(), getAddress_length()) == 0;
	}
	bool Address::operator!=(const Address& rhs)const
	{
		return !(*this == rhs);
	}

	//class Address:public static
	shared_ptr<Address> Address::Create(const sockaddr* address, socklen_t addrlen)
	{
		if (address == nullptr)
		{
			return nullptr;
		}

		shared_ptr<Address> result;
		switch (address->sa_family)
		{
		case AF_INET:
			result.reset(new IPv4Address(*(const sockaddr_in*)address));
			break;
		case AF_INET6:
			result.reset(new IPv6Address(*(const sockaddr_in6*)address));
			break;
		default:
			result.reset(new UnknowAddress(*address));
			break;
		}
		return result;
	}

	bool Address::Lookup(vector<shared_ptr<Address>>& result, const string& host, int family, int type, int protocol)
	{
		addrinfo hints;
		addrinfo* results = NULL, * next = NULL;
		hints.ai_flags = 0;
		hints.ai_family = family;
		hints.ai_socktype = type;
		hints.ai_protocol = protocol;
		hints.ai_addrlen = 0;
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;

		string node;
		const char* service = NULL;
		//检查ipv6地址是否携带service
		if (!host.empty() && host[0] == '[')
		{
			const char* end_ipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
			if (end_ipv6)
			{
				//检查越界
				if (*(end_ipv6 + 1) == ':')
				{
					service = end_ipv6 + 2;
				}
				node = host.substr(1, end_ipv6 - host.c_str() - 1);
			}
		}
		//检查node是否携带service
		if (node.empty())
		{
			service = (const char*)memchr(host.c_str(), ':', host.size());
			if (service)
			{
				if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1))
				{
					node = host.substr(0, service - host.c_str());
					++service;
				}
			}
		}

		if (node.empty())
		{
			node = host;
		}
		int error = getaddrinfo(node.c_str(), service, &hints, &results);
		if (error)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "Address::Lookup getaddrinfo(" << host << ", " << family << ", " << type
				<< ") error=" << error << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		next = results;
		while (next)
		{
			result.push_back(Create(next->ai_addr, (socklen_t)next->ai_addrlen));
			next = next->ai_next;
		}

		freeaddrinfo(results);
		return true;
	}
	shared_ptr<Address> Address::LookupAny(const string& host, int family,int type, int protocol)
	{
		vector<shared_ptr<Address>> result;
		if (Lookup(result, host, family, type, protocol))
		{
			return result[0];
		}
		return nullptr;
	}
	shared_ptr<IPAddress> Address::LookupAnyIPAddress(const string& host, int family ,int type, int protocol)
	{
		vector<shared_ptr<Address>> result;
		if (Lookup(result, host, family, type, protocol))
		{
			for (auto& i : result)
			{
				shared_ptr<IPAddress> v = dynamic_pointer_cast<IPAddress>(i);
				if (v)
				{
					return v;
				}
			}
		}
		return nullptr;
	}
	
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
	shared_ptr<IPAddress> IPAddress::Create(const char* address, uint32_t port)
	{
		addrinfo hints;
		addrinfo* results = NULL;
		memset(&hints, 0, sizeof(hints));
		//hints.ai_flags = AI_NUMERICHOST;
		hints.ai_flags = AI_CANONNAME;	//根据弹幕建议修改的
		hints.ai_family = AF_UNSPEC;

		int error = getaddrinfo(address, NULL, &hints, &results);
		if (error)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "IPAddress::Create(" << address << ", " << port
				<< ") errno=" << errno << " strerror=" << strerror(error);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			return nullptr;
		}

		try
		{
			shared_ptr<IPAddress> result =
				dynamic_pointer_cast<IPAddress>(Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
			if (result)
			{
				result->setPort(port);
			}
			freeaddrinfo(results);
			return result;
		}
		catch (...)
		{
			freeaddrinfo(results);
			return nullptr;
		}
	}

	//class IPv4Address :public
	IPv4Address::IPv4Address(uint32_t address, uint16_t port)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sin_family = AF_INET;
		m_address.sin_port = htons(port);
		m_address.sin_addr.s_addr = htonl(address);
	}
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
	ostream& IPv4Address::insert(ostream& os)const
	{
		uint32_t address = ntohl(m_address.sin_addr.s_addr);
		//&0xff用于保留最低的8位
		os << ((address >> 24) & 0xff) << "."
			<< ((address >> 16) & 0xff) << "."
			<< ((address >> 8) & 0xff) << "."
			<< (address & 0xff);
		os << ":" << ntohs(m_address.sin_port);
		return os;
	}

	shared_ptr<IPAddress> IPv4Address::broadcastAddress(uint32_t prefix_len)
	{
		if (prefix_len > 32)
		{
			return nullptr;
		}

		sockaddr_in broadcast_address(m_address);
		broadcast_address.sin_addr.s_addr |= htonl(CreateMask<uint32_t>(prefix_len));

		return shared_ptr<IPv4Address>(new IPv4Address(broadcast_address));
	}
	shared_ptr<IPAddress> IPv4Address::networdAddress(uint32_t prefix_len)
	{
		if (prefix_len > 32)
		{
			return nullptr;
		}

		sockaddr_in broadcast_address(m_address);
		broadcast_address.sin_addr.s_addr &= htonl(CreateMask<uint32_t>(prefix_len));

		return shared_ptr<IPv4Address>(new IPv4Address(broadcast_address));
	}
	shared_ptr<IPAddress> IPv4Address::subnetMask(uint32_t prefix_len)
	{
		sockaddr_in subnet_mask;
		memset(&subnet_mask, 0, sizeof(subnet_mask));
		subnet_mask.sin_family = AF_INET;
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
	shared_ptr<IPv4Address> IPv4Address::Create(const char* address, uint16_t port)
	{
		shared_ptr<IPv4Address> ipv4_address(new IPv4Address);
		ipv4_address->m_address.sin_port = htons(port);
		int addr = inet_pton(AF_INET, address, &ipv4_address->m_address.sin_addr);
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
	IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sin6_family = AF_INET6;
		m_address.sin6_port = htons(port);
		memcpy(&m_address.sin6_addr.s6_addr, address, 16);
	}
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
	ostream& IPv6Address::insert(ostream& os)const
	{
		os << "[";

		uint16_t* address = (uint16_t*)m_address.sin6_addr.s6_addr;
		bool used_zeros = false;

		for (size_t i = 0; i < 8; ++i)
		{
			if (address[i] == 0 && !used_zeros)
			{
				continue;
			}
			if (i > 0 && address[i - 1] == 0 && !used_zeros)
			{
				os << ":";
				used_zeros = true;
			}
			if (i > 0)
			{
				os << ":";
			}
			os << hex << (int)ntohs(address[i]) << dec;
		}
		
		if (!used_zeros && address[7] == 0)
		{
			os << "::";
		}

		os << "]:" << ntohs(m_address.sin6_port);
		return os;
	}

	shared_ptr<IPAddress> IPv6Address::broadcastAddress(uint32_t prefix_len)
	{
		sockaddr_in6 broadcast_address(m_address);
		broadcast_address.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
		for (size_t i = prefix_len / 8 + 1; i < 16; ++i)
		{
			broadcast_address.sin6_addr.s6_addr[i] = 0xff;
		}

		return shared_ptr<IPv6Address>(new IPv6Address(broadcast_address));
	}
	shared_ptr<IPAddress> IPv6Address::networdAddress(uint32_t prefix_len)
	{
		sockaddr_in6 netword_address(m_address);
		netword_address.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
		for (size_t i = prefix_len / 8 + 1; i < 16; ++i)
		{
			netword_address.sin6_addr.s6_addr[i] = 0x00;
		}

		return shared_ptr<IPv6Address>(new IPv6Address(netword_address));
	}
	shared_ptr<IPAddress> IPv6Address::subnetMask(uint32_t prefix_len)
	{
		sockaddr_in6 subnet_mask(m_address);
		memset(&subnet_mask, 0, sizeof(subnet_mask));
		subnet_mask.sin6_family = AF_INET6;
		subnet_mask.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);
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
	shared_ptr<IPv6Address> IPv6Address::Create(const char* address, uint16_t port)
	{
		shared_ptr<IPv6Address> ipv6_address(new IPv6Address);
		ipv6_address->m_address.sin6_port = htons(port);
		int addr = inet_pton(AF_INET6, address, &ipv6_address->m_address.sin6_addr);
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
	ostream& UnixAddress::insert(ostream& os)const
	{
		if (m_length > offsetof(sockaddr_un, sun_path) && m_address.sun_path[0]=='\0')
		{
			return os << "\\0" << string(m_address.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
		}
		return os << m_address.sun_path;
	}


	//class UnknowAddress :public
	UnknowAddress::UnknowAddress(int family)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sa_family = family;
	}
	UnknowAddress::UnknowAddress(const sockaddr& address)
	{
		m_address = address;
	}

	const sockaddr* UnknowAddress::getAddress()const
	{
		return &m_address;
	}
	sockaddr* UnknowAddress::getAddress()
	{
		return &m_address;
	}
	socklen_t UnknowAddress::getAddress_length()const
	{
		return sizeof(m_address);
	}
	ostream& UnknowAddress::insert(ostream& os)const
	{
		os << "[Unknow Address,family=" << m_address.sa_family << "]";
		return os;
	}
}
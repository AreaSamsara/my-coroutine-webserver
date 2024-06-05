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

	//class Address:public
	//����ַת��Ϊ�ַ���
	string Address::toString() const
	{
		stringstream ss;
		//�Ȼ�ȡ�ɶ��������ַ������������ٽ���ת��Ϊstring����
		getAddress_ostream(ss);
		return ss.str();
	}

	bool Address::operator<(const Address& right_address)const
	{
		//ȡ���ߵĽ�С������Ϊmemcmp�ĳ���
		socklen_t minlength = min(getAddress_length(), right_address.getAddress_length());
		//�ȱȽ��ڴ������ݵĴ�С
		int result = memcmp(getAddress(), right_address.getAddress(), minlength);
		if (result < 0)
		{
			return true;
		}
		else if (result > 0)
		{
			return false;
		}
		//���ڴ������ݵĴ�С��ȣ��ٱȽϵ�ַ����
		else if (getAddress_length() < right_address.getAddress_length())
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	bool Address::operator==(const Address& right_address)const
	{
		//���ҽ����ڴ������ݵĴ�С����ҵ�ַ�������ʱ����true
		return getAddress_length() == right_address.getAddress_length()
			&& memcmp(getAddress(), right_address.getAddress(), getAddress_length()) == 0;
	}
	bool Address::operator!=(const Address& right_address)const
	{
		return !(*this == right_address);
	}


	//class Address:public static
	//��sockaddrָ�봴��Address����
	shared_ptr<Address> Address::CreateAddress(const sockaddr* address, socklen_t addrlen)
	{
		//���sockaddrָ��Ϊ�գ�ֱ�ӷ���nullptr
		if (address == nullptr)
		{
			return nullptr;
		}

		shared_ptr<Address> result;
		//����address��Э������ö�Ӧ�Ĺ��캯������Address����
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


	//ͨ��host��ַ���ط�������������Address(���봫���������)
	bool Address::Lookup(vector<shared_ptr<Address>>& addresses, const string& host, int family, int type, int protocol)
	{
		//������������
		addrinfo hints;
		hints.ai_flags = 0;
		hints.ai_family = family;
		hints.ai_socktype = type;
		hints.ai_protocol = protocol;
		hints.ai_addrlen = 0;
		hints.ai_canonname = NULL;
		hints.ai_addr = NULL;

		//�������
		addrinfo* results = NULL;
		//��ַ�ַ����������������ţ�
		string address_str;
		//�˿ں��ַ�ָ��
		const char* port_char_ptr = NULL;

		//���host��ַ��Ϊ������'['��ͷ��˵����ΪЯ�������ŵ�IPv6��ַ������ipv6��ַ�Ƿ�Я���˿ں�
		if (!host.empty() && host[0] == '[')
		{
			//����host��ַ�ַ�����']'��һ�γ��ֵ�λ�ã���ΪIPv6��ַ���������˿ںţ���ĩβ
			const char* end_ipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
			//������ҳɹ�
			if (end_ipv6)
			{
				//�����ַĩβ����һ���ַ�Ϊ':'˵����host��ַЯ���˶˿ںţ���ȡ֮
				if (*(end_ipv6 + 1) == ':')
				{
					port_char_ptr = end_ipv6 + 2;
				}
				//����ַ����Ϊ���������ڵĲ���
				address_str = host.substr(1, end_ipv6 - host.c_str() - 1);
			}
		}

		//�����ַ�ַ�����Ϊ�գ�˵���䲻Я��������
		if (address_str.empty())
		{
			//����host��ַ�ַ�����':'��һ�γ��ֵ�λ�ã���Ϊ�˿ں��ַ�ָ���ǰһ��λ��
			port_char_ptr = (const char*)memchr(host.c_str(), ':', host.size());
			//������ҳɹ���˵����host��ַ�����˿ں�
			if (port_char_ptr)
			{
				//����ڽ������Ĳ����в�����':'��˵���������õ�ַ�Ͷ˿ں��ַ�ָ�루��������
				if (!memchr(port_char_ptr + 1, ':', host.c_str() + host.size() - port_char_ptr - 1))
				{
					//����ַ����Ϊ':'֮ǰ�Ĳ���
					address_str = host.substr(0, port_char_ptr - host.c_str());
					//���˿ں��ַ�ָ���ƶ�һλ��ʹ��ָ����ȷ��λ��
					++port_char_ptr;
				}
			}
			//����˵����host��ַ��Я���˿ںţ�ֱ�ӽ���ַ����Ϊhost��ַ����ʱ�˿ں��ַ�ָ��ΪĬ��ֵNULL��getaddrinfo()�����������������Ƕ˿ںţ�
			else
			{
				address_str = host;
			}
		}


		//����getaddrinfo()�������Խ���address����Ϣ���ɹ�����0��
		int error = getaddrinfo(address_str.c_str(), port_char_ptr, &hints, &results);
		//�������ʧ�ܣ���������false
		if (error != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "Address::Lookup getaddrinfo(" << host << ", " << family << ", " << type
				<< ") error=" << error << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		//�������������Address���󣬲����η���addresses��
		addrinfo* next_result = results;
		while (next_result)
		{
			addresses.push_back(CreateAddress(next_result->ai_addr, (socklen_t)next_result->ai_addrlen));
			next_result = next_result->ai_next;
		}

		//�����ַ��Ϣ�ṹ��
		freeaddrinfo(results);
		return true;
	}

	//ͨ��host��ַ���ط�����������һAddress
	shared_ptr<Address> Address::LookupAny(const string& host, int family,int type, int protocol)
	{
		vector<shared_ptr<Address>> addresses;
		//����Lookup()����������ɹ��򷵻ػ�ȡ�ĵ�һ��Address
		if (Lookup(addresses, host, family, type, protocol))
		{
			return addresses[0];
		}
		//���򷵻�nullptr
		return nullptr;
	}

	//ͨ��host��ַ���ط�����������һIPAddress
	shared_ptr<IPAddress> Address::LookupAnyIPAddress(const string& host, int family ,int type, int protocol)
	{
		vector<shared_ptr<Address>> addresses;
		//����Lookup()����������ɹ��򷵻ػ�ȡ�ĵ�һ���ܹ��ɹ�ͨ������ָ��ת��ת����IPAddress��Address
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
		//���򷵻�nullptr
		return nullptr;
	}
	

	//���ر�������������<������, ��ַ, ��������λ��>
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
					address = CreateAddress(next->ifa_addr, sizeof(sockaddr_in));
					uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
					prefix_length = CountBytes(netmask);
				}
					break;
				case AF_INET6:
				{
					address = CreateAddress(next->ifa_addr, sizeof(sockaddr_in6));
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

	//��ȡָ�������ĵ�ַ����������λ��
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
	//ͨ������,IP,������������IPAddress
	shared_ptr<IPAddress> IPAddress::CreateIPAddress(const char* address, const uint32_t port)
	{
		//������������
		addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		// ��ȡ�淶������
		//hints.ai_flags = AI_NUMERICHOST;
		hints.ai_flags = AI_CANONNAME;	//���ݵ�Ļ�����޸ĵ�
		// ָ����ַ��Ϊδָ������getaddrinfo������
		hints.ai_family = AF_UNSPEC;

		//�������
		addrinfo* results = NULL;

		//����getaddrinfo()�������Խ���address����Ϣ���ɹ�����0��
		int error = getaddrinfo(address, NULL, &hints, &results);
		//�������ʧ�ܣ���������nullptr
		if (error != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "IPAddress::Create(" << address << ", " << port
				<< ") errno=" << errno << " strerror=" << strerror(error);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			return nullptr;
		}

		//������ȡ�õ���Ϣ����IPAddress����
		try
		{
			//��֤����ָ��ת���İ�ȫ
			shared_ptr<IPAddress> ip_address = dynamic_pointer_cast<IPAddress>(Address::CreateAddress(results->ai_addr, (socklen_t)results->ai_addrlen));
			//��������ɹ���������˿�
			if (ip_address)
			{
				ip_address->setPort(port);
			}
			//����������������ش�����IPAddress����
			freeaddrinfo(results);
			return ip_address;
		}
		//�������ָ��ת�����������������������nullptr
		catch (...)
		{
			freeaddrinfo(results);
			return nullptr;
		}
	}





	//class IPv4Address :public
	//ͨ��IPv4�����Ƶ�ַ����IPv4Address
	IPv4Address::IPv4Address(const uint32_t address, const uint16_t port)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sin_family = AF_INET;
		m_address.sin_port = htons(port);
		m_address.sin_addr.s_addr = htonl(address);
	}
	
	//ʹ�õ��ʮ���Ƶ�ַ����IPv4Address
	IPv4Address::IPv4Address(const char* address, const uint16_t port)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sin_family = AF_INET;
		m_address.sin_port = htons(port);

		int return_value = inet_pton(AF_INET, address, &this->m_address.sin_addr);
		//���inet_pton()����ʧ���򱨴�
		if (return_value <= 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "IPv4Address::Create(" << address << ", " << port << ") return=" << return_value
				<< " errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		}
	}

	//��ȡ�ɶ��������ַ
	ostream& IPv4Address::getAddress_ostream(ostream& os)const
	{
		uint32_t address = ntohl(m_address.sin_addr.s_addr);
		//��IPv4�ĵ��ʮ���Ƶ�ַд�����У�&0xff���ڱ�����͵�8λ��
		os << ((address >> 24) & 0xff) << "."
			<< ((address >> 16) & 0xff) << "."
			<< ((address >> 8) & 0xff) << "."
			<< (address & 0xff);
		//���˿ں�д������
		os << ":" << ntohs(m_address.sin_port);
		return os;
	}

	//��ȡ�˿ں�
	uint16_t IPv4Address::getPort()const
	{
		return ntohs(m_address.sin_port);
	}
	//���ö˿ں�
	void IPv4Address::setPort(const uint16_t port)
	{
		m_address.sin_port = htons(port);
	}

	//��ȡ�õ�ַ�Ĺ㲥��ַ
	shared_ptr<IPAddress> IPv4Address::broadcastAddress(const uint32_t prefix_len)
	{
		//������ڸ�λ����Ӧ�ó���32λ�����򷵻�nullptr
		if (prefix_len > 32)
		{
			return nullptr;
		}

		//����ǰ��ַ��prefix_lenλ����ȡ�򣬵õ��㲥��ַ
		sockaddr_in broadcast_address(m_address);
		broadcast_address.sin_addr.s_addr |= htonl(CreateMask<uint32_t>(prefix_len));

		return shared_ptr<IPv4Address>(new IPv4Address(broadcast_address));
	}

	//��ȡ�õ�ַ������
	shared_ptr<IPAddress> IPv4Address::networdAddress(const uint32_t prefix_len)
	{
		//������ڸ�λ����Ӧ�ó���32λ�����򷵻�nullptr
		if (prefix_len > 32)
		{
			return nullptr;
		}

		//����ǰ��ַ���������루����ķ��룩ȡ�룬�õ�����
		sockaddr_in broadcast_address(m_address);
		broadcast_address.sin_addr.s_addr &= ~htonl(CreateMask<uint32_t>(prefix_len));

		return shared_ptr<IPv4Address>(new IPv4Address(broadcast_address));
	}

	//��ȡ���������ַ
	shared_ptr<IPAddress> IPv4Address::subnetMask(const uint32_t prefix_len)
	{
		//������ڸ�λ����Ӧ�ó���32λ�����򷵻�nullptr
		if (prefix_len > 32)
		{
			return nullptr;
		}

		//�������������ֻ����Э���壬��������Ҫ�ĳ�Ա�ÿգ����Ч��
		sockaddr_in subnet_mask;
		memset(&subnet_mask, 0, sizeof(subnet_mask));
		subnet_mask.sin_family = AF_INET;

		//prefix_lenλ����ķ��뼴Ϊ��������
		subnet_mask.sin_addr.s_addr = ~htonl(CreateMask<uint32_t>(prefix_len));

		return shared_ptr<IPv4Address>(new IPv4Address(subnet_mask));
	}

	







	//class IPv6Address : public
	IPv6Address::IPv6Address()
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sin6_family = AF_INET6;
	}
	//ͨ��IPv6�����Ƶ�ַ����IPv6Address
	IPv6Address::IPv6Address(const uint8_t address[16], const uint16_t port)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sin6_family = AF_INET6;
		m_address.sin6_port = htons(port);
		memcpy(&m_address.sin6_addr.s6_addr, address, 16);
	}
	//ͨ��IPv6��ַ�ַ�������IPv6Address
	IPv6Address::IPv6Address(const char* address, const  uint16_t port)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sin6_family = AF_INET6;
		m_address.sin6_port = htons(port);

		int return_value = inet_pton(AF_INET6, address, &this->m_address.sin6_addr);
		//���inet_pton()����ʧ���򱨴�
		if (return_value <= 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "IPv6Address::Create(" << address << ", " << port << ") return=" << return_value
				<< " errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		}
	}

	//��ȡ�ɶ��������ַ
	ostream& IPv6Address::getAddress_ostream(ostream& os)const
	{
		os << "[";

		//��16λ��ÿ����ð�ż��4��16������ռ16λ��Ϊ��������IPv6��ַ��s6_addr����洢�Ǵӵ�ַ�ĸ�λ��ʼ�ģ�
		uint16_t* address = (uint16_t*)m_address.sin6_addr.s6_addr;
		//�Ƿ��Ѿ�ʹ�ù�����ѹ����ÿ��IPv6��ַ��ֻ����ʹ��һ���Ա������壩
		bool used_zeros = false;

		//���δ���ÿһ�ε�ַ
		for (size_t i = 0; i < 8; ++i)
		{
			//�����״������������������㣬ֱ����������ж�Ϊֹ
			if (address[i] == 0 && !used_zeros)
			{
				continue;
			}
			//���״��������������㱻�жϺ�ʹ����ѹ���������һ��ð�����γ�˫ð�ţ�
			if (i > 0 && address[i - 1] == 0 && !used_zeros)
			{
				os << ":";
				used_zeros = true;
			}
			//�������Ĳ��ǵ�һ��16λ�������ð��
			if (i > 0)
			{
				os << ":";
			}
			//��16���ƽ��öε�ַ�������
			os << hex << (int)ntohs(address[i]) << dec;
		}
		
		//����״�����������������ĩβ����ֱ����ĩβ���˫ð��
		if (!used_zeros && address[7] == 0)
		{
			os << "::";
		}

		os << "]:" << ntohs(m_address.sin6_port);
		return os;
	}

	//��ȡ�˿ں�
	uint16_t IPv6Address::getPort()const
	{
		return ntohs(m_address.sin6_port);
	}
	//���ö˿ں�
	void IPv6Address::setPort(const uint16_t port)
	{
		m_address.sin6_port = htons(port);
	}

	//��ȡ�õ�ַ�Ĺ㲥��ַ
	shared_ptr<IPAddress> IPv6Address::broadcastAddress(const uint32_t prefix_len)
	{
		//����ǰ��ַ��prefix_lenλ����ȡ�򣬵õ��㲥��ַ
		sockaddr_in6 broadcast_address(m_address);
		//ֻ���ٽ���ֽڽ��л����
		broadcast_address.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
		//�ٽ��ֽ��Ժ���ֽ�ֱ���øߣ�s6_addr����洢�Ǵӵ�ַ�ĸ�λ��ʼ�ģ�
		for (size_t i = prefix_len / 8 + 1; i < 16; ++i)
		{
			broadcast_address.sin6_addr.s6_addr[i] = 0xff;
		}

		return shared_ptr<IPv6Address>(new IPv6Address(broadcast_address));
	}

	//��ȡ�õ�ַ������
	shared_ptr<IPAddress> IPv6Address::networdAddress(const uint32_t prefix_len)
	{
		//����ǰ��ַ���������루����ķ��룩ȡ�룬�õ�����
		sockaddr_in6 netword_address(m_address);
		//ֻ���ٽ���ֽڽ��������
		netword_address.sin6_addr.s6_addr[prefix_len / 8] &= ~CreateMask<uint8_t>(prefix_len % 8);
		//�ٽ��ֽ��Ժ���ֽ�ֱ���õͣ�s6_addr����洢�Ǵӵ�ַ�ĸ�λ��ʼ�ģ�
		for (size_t i = prefix_len / 8 + 1; i < 16; ++i)
		{
			netword_address.sin6_addr.s6_addr[i] = 0x00;
		}

		return shared_ptr<IPv6Address>(new IPv6Address(netword_address));
	}

	//��ȡ���������ַ
	shared_ptr<IPAddress> IPv6Address::subnetMask(const uint32_t prefix_len)
	{
		//prefix_lenλ����ķ��뼴Ϊ��������
		sockaddr_in6 subnet_mask;
		//�������������ֻ����Э���壬��������Ҫ�ĳ�Ա�ÿգ����Ч��
		memset(&subnet_mask, 0, sizeof(subnet_mask));
		subnet_mask.sin6_family = AF_INET6;

		//ֻ���ٽ���ֽڽ���ȡ������
		subnet_mask.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);
		//�ٽ��ֽ���ǰ���ֽ�ֱ���øߣ�s6_addr����洢�Ǵӵ�ַ�ĸ�λ��ʼ�ģ�
		for (size_t i = 0; i < prefix_len / 8; ++i)
		{
			subnet_mask.sin6_addr.s6_addr[i] = 0xff;
		}

		return shared_ptr<IPv6Address>(new IPv6Address(subnet_mask));
	}

	






	//class UinxAddress :public
	UnixAddress::UnixAddress()
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sun_family = AF_UNIX;
		m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
	}

	//ͨ��·���ַ�������UnixAddress
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

	//��ȡ�ɶ��������ַ
	ostream& UnixAddress::getAddress_ostream(ostream& os)const
	{
		if (m_length > offsetof(sockaddr_un, sun_path) && m_address.sun_path[0]=='\0')
		{
			return os << "\\0" << string(m_address.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
		}
		return os << m_address.sun_path;
	}


	//class UinxAddress :private variable
	//���·������
	const size_t UnixAddress::MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;




	//class UnknowAddress :public
	UnknownAddress::UnknownAddress(int family)
	{
		memset(&m_address, 0, sizeof(m_address));
		m_address.sa_family = family;
	}

	//��ȡ�ɶ��������ַ
	ostream& UnknownAddress::getAddress_ostream(ostream& os)const
	{
		os << "[Unknow Address,family=" << m_address.sa_family << "]";
		return os;
	}
}
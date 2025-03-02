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
	using std::multimap;
	using std::ostream;
	using std::pair;
	using std::shared_ptr;
	using std::string;
	using std::vector;

	/*
	 * ���ϵ��
	 * Address���IPAddress�඼�ǳ�����࣬���߹��м̳���ǰ�ߣ������ڲ��п����ڴ������������ľ�̬���з���
	 * IPv4Address���IPv6Address�๫�м̳���IPAddress�࣬ UnixAddress���UnknownAddress��ֱ�ӹ��м̳���Address��
	 */
	/*
	 * ��ַϵͳ���÷�����
	 * 1.����ͨ�������Ƶ�ַ����ַ�ַ������˿ںš���ַ�ṹ�����Ϣֱ�ӵ��ö�Ӧ��Ĺ��캯��������ַ����
	 * 2.Ҳ����ʹ��Address��ĸ���Lookup()��������̬������ͨ���ַ������ҵ�ַ��
	 * ���ҵ��Ժ�Lookup()�����ڲ��Զ�����CreateAddress()������Ӧ��Address�༰����������󣨴˺�Ӧ���������ö˿ںţ�
	 */

	class Address;		  // ��ַ�ࣨ������ࣩ
	class IPAddress;	  // IP��ַ�ࣨ������ࣩ�����м̳���Address��
	class IPv4Address;	  // IPv4��ַ�࣬���м̳���IPAddress��
	class IPv6Address;	  // IPv6��ַ�࣬���м̳���IPAddress��
	class UnixAddress;	  // Unix��ַ�࣬���м̳���Address��
	class UnknownAddress; // δ֪��ַ�࣬���м̳���Address��

	// ��ַ�ࣨ������ࣩ
	class Address
	{
	public:
		virtual ~Address() {}

		// ��ȡֻ�����ַָ��
		virtual const sockaddr *getAddress() const = 0;
		// ��ȡ��ַָ��
		virtual sockaddr *getAddress() = 0;
		// ��ȡ��ַ�ĳ���
		virtual socklen_t getAddress_length() const = 0;
		// ��ȡ�ɶ��������ַ
		virtual ostream &getAddress_ostream(ostream &os) const = 0;

		// ����Э����
		int getFamily() const { return getAddress()->sa_family; }
		// ����ַת��Ϊ�ַ���
		string toString() const;

		// ���رȽ������
		bool operator<(const Address &right_address) const;
		bool operator==(const Address &right_address) const;
		bool operator!=(const Address &right_address) const;

	public:
		// ��sockaddrָ�봴��Address����
		static shared_ptr<Address> CreateAddress(const sockaddr *address, socklen_t addrlen);

		// ͨ��host��ַ���ط�������������Address(���봫���������)
		static bool Lookup(vector<shared_ptr<Address>> &addresses, const string &host,
						   int family = AF_INET, int type = 0, int protocol = 0); // Ĭ�Ϸ���IPv4�ĵ�ַ����Ϊ��ͨ�����ٶȿ���IPv6
		// ͨ��host��ַ���ط�����������һAddress
		static shared_ptr<Address> LookupAny(const string &host, int family = AF_INET,
											 int type = 0, int protocol = 0);
		// ͨ��host��ַ���ط�����������һIPAddress
		static shared_ptr<IPAddress> LookupAnyIPAddress(const string &host, int family = AF_INET,
														int type = 0, int protocol = 0);

		// ���ر�������������<������, ��ַ, ��������λ��>
		static bool GetInterfaceAddresses(multimap<string, pair<shared_ptr<Address>, uint32_t>> &result,
										  int family = AF_INET);
		// ��ȡָ�������ĵ�ַ����������λ��
		static bool GetInterfaceAddresses(vector<pair<shared_ptr<Address>, uint32_t>> &result,
										  const string &iface, int family = AF_INET);

	private:
		// ������������Ʊ�����1��λ��
		template <class T>
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

	// IP��ַ�ࣨ������ࣩ�����м̳���Address��
	class IPAddress : public Address
	{
	public:
		// ��ȡ�õ�ַ�Ĺ㲥��ַ
		virtual shared_ptr<IPAddress> broadcastAddress(const uint32_t prefix_len) = 0;
		// ��ȡ�õ�ַ������
		virtual shared_ptr<IPAddress> networdAddress(const uint32_t prefix_len) = 0;
		// ��ȡ���������ַ
		virtual shared_ptr<IPAddress> subnetMask(const uint32_t prefix_len) = 0;

		// ��ȡ�˿ں�
		virtual uint16_t getPort() const = 0;
		// ���ö˿ں�
		virtual void setPort(const uint16_t port) = 0;

	public:
		// ͨ������,IP,������������IPAddress
		static shared_ptr<IPAddress> CreateIPAddress(const char *address, const uint32_t port = 0);

	protected:
		// �������루��͵�bitsλΪ1��
		template <class T>
		static T CreateMask(const uint32_t bits)
		{
			return (1 << (sizeof(T) * 8 - bits)) - 1;
		}
	};

	// IPv4��ַ�࣬���м̳���IPAddress��
	class IPv4Address : public IPAddress
	{
	public:
		// ͨ��IPv4�����Ƶ�ַ����IPv4Address
		IPv4Address(const uint32_t address = INADDR_ANY, const uint16_t port = 0);
		// ͨ��sockaddr_in����IPv4Address
		IPv4Address(const sockaddr_in &address) { m_address = address; }
		// ʹ�õ��ʮ���Ƶ�ַ����IPv4Address
		IPv4Address(const char *address, const uint16_t port);

		// ��ȡֻ�����ַָ��
		virtual const sockaddr *getAddress() const override { return (sockaddr *)&m_address; }
		// ��ȡ��ַָ��
		virtual sockaddr *getAddress() override { return (sockaddr *)&m_address; }
		// ��ȡ��ַ�ĳ���
		virtual socklen_t getAddress_length() const override { return sizeof(m_address); }
		// ��ȡ�ɶ��������ַ
		virtual ostream &getAddress_ostream(ostream &os) const override;

		// ��ȡ�˿ں�
		virtual uint16_t getPort() const override;
		// ���ö˿ں�
		virtual void setPort(const uint16_t port) override;

		// ��ȡ�õ�ַ�Ĺ㲥��ַ
		virtual shared_ptr<IPAddress> broadcastAddress(const uint32_t prefix_len) override;
		// ��ȡ�õ�ַ������
		virtual shared_ptr<IPAddress> networdAddress(const uint32_t prefix_len) override;
		// ��ȡ���������ַ
		virtual shared_ptr<IPAddress> subnetMask(const uint32_t prefix_len) override;

	private:
		// IPv4��ַ�ṹ��
		sockaddr_in m_address;
	};

	// IPv6��ַ�࣬���м̳���IPAddress��
	class IPv6Address : public IPAddress
	{
	public:
		IPv6Address();
		// ͨ��IPv6�����Ƶ�ַ����IPv6Address
		IPv6Address(const uint8_t address[16], const uint16_t port = 0);
		// ͨ��sockaddr_in6����IPv6Address
		IPv6Address(const sockaddr_in6 &address) { m_address = address; }
		// ͨ��IPv6��ַ�ַ�������IPv6Address
		IPv6Address(const char *address, uint16_t port);

		// ��ȡֻ�����ַָ��
		virtual const sockaddr *getAddress() const override { return (sockaddr *)&m_address; }
		// ��ȡ��ַָ��
		virtual sockaddr *getAddress() override { return (sockaddr *)&m_address; }
		// ��ȡ��ַ�ĳ���
		virtual socklen_t getAddress_length() const override { return sizeof(m_address); }
		// ��ȡ�ɶ��������ַ
		virtual ostream &getAddress_ostream(ostream &os) const override;

		// ��ȡ�˿ں�
		virtual uint16_t getPort() const override;
		// ���ö˿ں�
		virtual void setPort(const uint16_t port) override;

		// ��ȡ�õ�ַ�Ĺ㲥��ַ
		virtual shared_ptr<IPAddress> broadcastAddress(const uint32_t prefix_len) override;
		// ��ȡ�õ�ַ������
		virtual shared_ptr<IPAddress> networdAddress(const uint32_t prefix_len) override;
		// ��ȡ���������ַ
		virtual shared_ptr<IPAddress> subnetMask(const uint32_t prefix_len) override;

	private:
		// IPv6��ַ�ṹ��
		sockaddr_in6 m_address;
	};

	// Unix��ַ�࣬���м̳���Address��
	class UnixAddress : public Address
	{
	public:
		UnixAddress();
		// ͨ��·���ַ�������UnixAddress
		UnixAddress(const string &path);

		// ��ȡֻ�����ַָ��
		virtual const sockaddr *getAddress() const override { return (sockaddr *)&m_address; }
		// ��ȡ��ַָ��
		virtual sockaddr *getAddress() override { return (sockaddr *)&m_address; }
		// ��ȡ��ַ�ĳ���
		virtual socklen_t getAddress_length() const override { return m_length; }
		// ��ȡ�ɶ��������ַ
		virtual ostream &getAddress_ostream(ostream &os) const override;

		// ���õ�ַ����
		void setAddress_length(const uint32_t address_length) { m_length = address_length; }

	private:
		// Unix��ַ�ṹ��
		sockaddr_un m_address;
		// Unix��ַ����
		socklen_t m_length;

	private:
		// ���·������
		static const size_t MAX_PATH_LEN;
	};

	// δ֪��ַ�࣬���м̳���Address��
	class UnknownAddress : public Address
	{
	public:
		UnknownAddress(const int family);
		// ͨ��sockaddr����UnknownAddress
		UnknownAddress(const sockaddr &address) { m_address = address; }

		// ��ȡֻ�����ַָ��
		virtual const sockaddr *getAddress() const override { return &m_address; }
		// ��ȡ��ַָ��
		virtual sockaddr *getAddress() override { return &m_address; }
		// ��ȡ��ַ�ĳ���
		virtual socklen_t getAddress_length() const override { return sizeof(m_address); }
		// ��ȡ�ɶ��������ַ
		virtual ostream &getAddress_ostream(ostream &os) const override;

	private:
		// ͨ�õ�ַ�ṹ��
		sockaddr m_address;
	};
}
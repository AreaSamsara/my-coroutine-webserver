#pragma once
#include <memory>
#include <string>
#include "tcp-ip/address.h"

namespace UriSpace
{
	using namespace AddressSpace;
	using std::ostream;
	using std::shared_ptr;
	using std::string;

	/*
		 foo://user@sylar.com:8042/over/there?name=ferret#nose
		   \_/   \______________/\_________/ \_________/ \__/
			|           |            |            |        |
		 scheme     authority       path        query   fragment
	*/

	class Uri
	{
	public:
		Uri();

		const string &getScheme() const { return m_scheme; }
		const string &getAuthority() const { return m_userinfo; }
		const string &getHost() const { return m_host; }
		const string &getPath() const;
		const string &getQuery() const { return m_query; }
		const string &getFragment() const { return m_fragment; }
		int32_t getPort() const;

		void setScheme(const string &scheme) { m_scheme = scheme; }
		void setUserinfo(const string &userinfo) { m_userinfo = userinfo; }
		void setHost(const string &host) { m_host = host; }
		void setPath(const string &path) { m_path = path; }
		void setQuery(const string &query) { m_query = query; }
		void setFragment(const string &fragment) { m_fragment = fragment; }
		void setPort(const int32_t port) { m_port = port; }

		ostream &dump(ostream &os) const;
		string toString() const;

		shared_ptr<Address> createAddress() const;

	public:
		static shared_ptr<Uri> Create(const string &uristr);

	private:
		bool isDefaultPort() const;

	private:
		string m_scheme;
		string m_userinfo; // aka authority
		string m_host;
		string m_path;
		string m_query;
		string m_fragment;

		int32_t m_port;
	};
}
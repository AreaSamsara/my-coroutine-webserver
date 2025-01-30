#pragma once
#include <memory>
#include "iomanager.h"
#include "socket.h"
#include "address.h"
#include "common/noncopyable.h"

namespace TcpServerSpace
{
	using namespace IOManagerSpace;
	using namespace SocketSpace;
	using namespace AddressSpace;
	using namespace NoncopyableSpace;

	class TcpServer : private Noncopyable, public enable_shared_from_this<TcpServer>
	{
	public:
		TcpServer(IOManager *iomanager = IOManager::GetThis(), IOManager *accept_iomanager = IOManager::GetThis());
		virtual ~TcpServer();

		virtual bool bind(shared_ptr<Address> address);
		virtual bool bind(const vector<shared_ptr<Address>> &addresses, vector<shared_ptr<Address>> &addresses_fail);
		virtual bool start();
		virtual void stop();

		uint64_t getRead_timeout() const { return m_receive_timeout; }
		string getName() const { return m_name; }
		bool isStopped() const { return m_is_stopped; }

		void setRead_timeout(const uint64_t read_timeout) { m_receive_timeout = read_timeout; }
		void setName(const string name) { m_name = name; }

	protected:
		virtual void handleClient(shared_ptr<Socket> client_socket);
		virtual void startAccept(shared_ptr<Socket> socket);

	private:
		vector<shared_ptr<Socket>> m_sockets;
		IOManager *m_iomanager;
		IOManager *m_accept_iomanager;
		uint64_t m_receive_timeout;
		string m_name;
		bool m_is_stopped;
	};

}
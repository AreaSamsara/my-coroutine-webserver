#pragma once
#include <memory>
#include "concurrent/iomanager.h"
#include "tcp-ip/socket.h"
#include "tcp-ip/address.h"
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
		// 绑定并监听列表中的地址
		virtual bool bind(const vector<shared_ptr<Address>> &addresses, vector<shared_ptr<Address>> &addresses_fail);
		// 启动服务器
		virtual bool start();
		// 停止服务器
		virtual void stop();

		// 获取私有变量
		uint64_t getRead_timeout() const { return m_receive_timeout; }
		string getName() const { return m_name; }
		bool isStopped() const { return m_is_stopped; }

		// 修改私有变量
		void setRead_timeout(const uint64_t read_timeout) { m_receive_timeout = read_timeout; }
		void setName(const string name) { m_name = name; }

	protected:
		// 处理客户端请求（只打印日志，不作处理）
		virtual void handleClient(shared_ptr<Socket> client_socket);
		// 令当前socket开始接收请求
		virtual void startAccept(shared_ptr<Socket> socket);

	private:
		// socket列表
		vector<shared_ptr<Socket>> m_sockets;
		// 用于处理请求的IO管理者
		IOManager *m_handle_iomanager;
		// 用于接收请求的IO管理者
		IOManager *m_accept_iomanager;
		// 接收超时时间
		uint64_t m_receive_timeout;
		// 服务器名称
		string m_name;
		// 是否已经停止
		bool m_is_stopped;
	};

}
#pragma once
#include "io/stream.h"
#include "socket.h"

namespace SocketStreamSpace
{
	using namespace StreamSpace;
	using namespace SocketSpace;

	class SocketStream : public Stream
	{
	public:
		SocketStream(shared_ptr<Socket> socket, const bool is_socket_owner = true);
		~SocketStream();

		virtual int read(void *buffer, const size_t read_length) override;
		virtual int read(shared_ptr<ByteArray> bytearray, const size_t read_length) override;

		virtual int write(const void *buffer, const size_t write_length) override;
		virtual int write(shared_ptr<ByteArray> bytearray, const size_t write_length) override;

		virtual void close() override;

		shared_ptr<Socket> getSocket() const { return m_socket; }

	protected:
		shared_ptr<Socket> m_socket;
		bool m_is_socket_owner;
	};
}
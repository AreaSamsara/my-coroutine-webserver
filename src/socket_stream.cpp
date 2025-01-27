#include "socket_stream.h"

namespace SocketStreamSpace
{
	using namespace StreamSpace;
	using namespace SocketSpace;

	SocketStream::SocketStream(shared_ptr<Socket> socket, const bool is_socket_owner)
		:m_socket(socket), m_is_socket_owner(is_socket_owner){}
	SocketStream::~SocketStream()
	{
		if (m_is_socket_owner && m_socket)
		{
			m_socket->close();
		}
	}

	int SocketStream::read(void* buffer, const size_t read_length)
	{
		if (m_socket && m_socket->isConnected())
		{
			return m_socket->recv(buffer, read_length);
		}
		else
		{
			return -1;
		}
	}
	int SocketStream::read(shared_ptr<ByteArray> bytearray, const size_t read_length)
	{
		if (m_socket && m_socket->isConnected())
		{
			vector<iovec> iovectors;
			bytearray->getWriteBuffers(iovectors, read_length);
			int return_value=m_socket->recv(&iovectors[0], iovectors.size());
			if (return_value > 0)
			{
				bytearray->setPosition(bytearray->getPosition() + return_value);
			}
			return return_value;
		}
		else
		{
			return -1;
		}
	}

	int SocketStream::write(const void* buffer, const size_t write_length)
	{
		if (m_socket && m_socket->isConnected())
		{
			return m_socket->send(buffer, write_length);
		}
		else
		{
			return -1;
		}
	}
	int SocketStream::write(shared_ptr<ByteArray> bytearray, const size_t write_length)
	{
		if (m_socket && m_socket->isConnected())
		{
			vector<iovec> iovectors;
			bytearray->getReadBuffers(iovectors, write_length, bytearray->getPosition());
			int return_value = m_socket->recv(&iovectors[0], iovectors.size());
			if (return_value > 0)
			{
				bytearray->setPosition(bytearray->getPosition() + return_value);
			}
			return return_value;
		}
		else
		{
			return -1;
		}
	}

	void SocketStream::close()
	{
		if (m_socket)
		{
			m_socket->close();
		}
	}
}
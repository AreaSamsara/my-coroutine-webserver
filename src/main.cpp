#include "tcp-ip/tcp-server.h"
#include "common/log.h"
#include "common/singleton.h"
#include "common/utility.h"
#include "io/bytearray.h"

using namespace LogSpace;
using namespace SingletonSpace;
using namespace UtilitySpace;
using namespace TcpServerSpace;
using namespace ByteArraySpace;

class EchoServer : public TcpServer
{
public:
	// 服务器接受数据的类型
	enum Type
	{
		TEXT = 1,
		BINARY = 0
	};
	EchoServer(const Type type);
	void handleClient(shared_ptr<Socket> client_socket) override;

private:
	Type m_type = TEXT;
};

EchoServer::EchoServer(const EchoServer::Type type) : m_type(type) {}

void EchoServer::handleClient(shared_ptr<Socket> client_socket)
{
	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
	log_event->getSstream() << "handleClient: " << client_socket;
	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);

	shared_ptr<ByteArray> bytearray(new ByteArray);
	while (true)
	{
		bytearray->clear();
		vector<iovec> iovectors;
		bytearray->getWriteBuffers(iovectors, 1024);

		int return_value = client_socket->recv(&iovectors[0], iovectors.size());
		if (return_value == 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "client close: " << client_socket;
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
			break;
		}
		else if (return_value < 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "client error,return_value: " << return_value
									<< " errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			break;
		}
		else
		{
			bytearray->setPosition(bytearray->getPosition() + return_value);

			bytearray->setPosition(0);
			// text
			if (m_type == Type::TEXT)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << bytearray->toString();
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
			}
			// binary
			else
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << bytearray->toHexString();
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
			}
		}
	}
}

void run(const EchoServer::Type type)
{
	shared_ptr<EchoServer> echo_server(new EchoServer(type));
	auto address = Address::LookupAny("0.0.0.0:8020");
	while (!echo_server->bind(address))
	{
		sleep(2);
	}
	echo_server->start();
}

int main(int argc, char **argv)
{
	// 如果参数小于2个，终止程序并提示
	if (argc < 2)
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
		return 0;
	}

	// 服务器接受数据的类型默认设置为文本
	EchoServer::Type type = EchoServer::TEXT;

	// 根据第二个参数设置类型
	if (strcmp(argv[1], "-b") == 0)
	{
		type = EchoServer::BINARY;
	}

	IOManager iomanager;
	iomanager.schedule(std::bind(&run, type));
	return 0;
}
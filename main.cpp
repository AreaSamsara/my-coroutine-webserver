#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "hook.h"
#include "iomanager.h"

using namespace IOManagerSpace;

const char SERVER_IP[] = "93.184.215.14";	//example.com
//const char SERVER_IP[] = "110.242.68.66";	//baidu.com


void test_sleep()
{
	IOManager iomanager(1);

	iomanager.schedule([]()
		{
			sleep(2);
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "sleep 2s";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
		}
	);

	iomanager.schedule([]()
		{
			sleep(3);
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "sleep 3s";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
		}
	);

	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
	log_event->getSstream() << "test sleep";
	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
}

void test_socket()
{
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	//将server_socket设置为非阻塞状态
	//fcntl(server_socket, F_SETFL, O_NONBLOCK);

	sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr.s_addr);

	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
	log_event->getSstream() << "begin connect";
	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

	int return_value = connect(server_socket, (const sockaddr*)(&server_addr), sizeof(server_addr));

	log_event->setSstream("");
	log_event->getSstream() << "connect return_value=" << return_value << " errno=" << errno << " server_socket=" << server_socket;
	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

	if (return_value)
	{
		return;
	}

	const char data[] = "GET / HTTP/1.0\r\n\r\n";
	return_value = send(server_socket, data, sizeof(data), 0);

	log_event->setSstream("");
	log_event->getSstream() << "send return_value=" << return_value << " errno=" << errno;
	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

	if (return_value <= 0)
	{
		return;
	}

	string buffer;
	buffer.resize(4096);

	return_value = recv(server_socket, &buffer[0], buffer.size(), 0);
	log_event->setSstream("");
	log_event->getSstream() << "recv return_value=" << return_value << " errno=" << errno;
	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

	if (return_value <= 0)
	{
		return;
	}

	buffer.resize(return_value);
	log_event->setSstream("");
	log_event->getSstream() << buffer;
	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
}

int main(int argc, char** argv)
{
	//test_sleep();
	//test_socket();
	IOManager iomanager;
	iomanager.schedule(test_socket);
	//iomanager.schedule(test_sleep);
	return 0;
}
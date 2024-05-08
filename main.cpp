#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "iomanager.h"


using namespace IOManagerSpace;

void test_fiber()
{
	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
	log_event->getSstream() << "test_fiber";
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	//将server_socket设置为非阻塞状态
	fcntl(server_socket, F_SETFL, O_NONBLOCK);

	sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr.s_addr);

	//如果成功连接
	if (connect(server_socket, (sockaddr*)(&server_addr), sizeof(server_addr))==0)
	{

	}
	//如果connect()函数正在进行但还未完成
	else if (errno == EINPROGRESS)
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		log_event->getSstream() << "add event errno=" << errno << " " << strerror(errno);
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

		IOManager::GetThis()->addEvent(server_socket, IOManager::READ, []()
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "read callback";
				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
			}
		);

		IOManager::GetThis()->addEvent(server_socket, IOManager::WRITE, [server_socket]()
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "write callback";
				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

				IOManager::GetThis()->settleEvent(server_socket, IOManager::READ);
				close(server_socket);
			}
		);
	}
	//否则说明出现了其他错误
	else
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		log_event->getSstream() << "else" << errno << " " << strerror(errno);
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
	}

}

void test1()
{
	//IOManager iom(1,true,"test");
	IOManager iom(2, false, "test");
	iom.schedule(&test_fiber);
}


void timer_callback(shared_ptr<Timer> timer)
{
	static int i = 0;
	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
	log_event->getSstream() << "hello timer i=" << i;
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

	IOManager* iomanager= IOManager::GetThis();
	if (++i == 3)
	{
		//iomanager->getTimer_manager()->resetRun_cycle(timer, 2000, true);
		iomanager->getTimer_manager()->cancel(timer);
	}
}


void test_timer()
{
	shared_ptr<IOManager> iomanager(new IOManager(2, true));
	
	shared_ptr<Timer> s_timer(new Timer(1000, nullptr, true));
	s_timer->setCallback(bind(timer_callback, s_timer));

	iomanager->addTimer(s_timer);
}

int main(int argc, char** argv)
{
	//test1();
	test_timer();
	return 0;
}
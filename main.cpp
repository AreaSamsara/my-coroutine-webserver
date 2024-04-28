#include "log.h"
#include "singleton.h"
#include "utility.h"
#include "fiber.h"
#include "scheduler.h"

using namespace SingletonSpace;
using namespace LogSpace;
using namespace FiberSpace;
using namespace UtilitySpace;
using namespace ThreadSpace;
using namespace SchedulerSpace;

void test_fiber()
{
	static int s_count = 5;

	shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
	event->getSstream() << "test in fiber s_count=" << s_count;
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
	
	sleep(1);
	
	if (--s_count >= 0)
	{
		Scheduler::GetThis()->schedule(&test_fiber);
		//Scheduler::GetThis()->schedule(&test_fiber,GetThread_id());
	}
}

int main(int argc, char** argv) 
{
	shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
	event->setSstream("main begin");
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

	//Scheduler sche(3,true,"test");
	Scheduler sche(3, false, "test");

	sche.start();

	sleep(2);

	event->setSstream("schedule");
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
	sche.schedule(&test_fiber);

	sche.stop();

	event->setSstream("main end");
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
	
	return 0;
}
//#include "log.h"
//#include "singleton.h"
//#include "utility.h"
//#include "fiber.h"
//
//using namespace SingletonSpace;
//using namespace LogSpace;
//using namespace FiberSpace;
//using namespace UtilitySpace;
//using namespace ThreadSpace;
//
//void run_in_fiber()
//{
//	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//
//	event->getSstream() << "run_in_fiber begin";
//	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//
//	Fiber::YieldTOHold();
//
//	event->setSstream("run_in_fiber end");
//	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//
//	Fiber::YieldTOHold();
//}
//
//void test_fiber()
//{
//	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//	event->setSstream("main begin");
//	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//
//
//	Fiber::GetThis();
//
//	event->setSstream("fiber test begin");
//	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//
//	shared_ptr<Fiber> fiber(new Fiber(run_in_fiber));
//
//	fiber->swapIn();
//
//	event->setSstream("after swapIn()");
//	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//
//	fiber->swapIn();
//
//	event->setSstream("fiber test end");
//	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//
//	fiber->swapIn();
//
//	event->setSstream("main end");
//	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//}
//
//int main(int argc, char** argv)
//{
//	vector<shared_ptr<Thread>> threads;
//
//	for (int i = 0; i < 3; ++i)
//	{
//		threads.push_back(shared_ptr<Thread>(new Thread(&test_fiber, "name_" + to_string(i))));
//	}
//
//	for (auto thread : threads)
//	{
//		thread->join();
//	}
//
//	return 0;
//}
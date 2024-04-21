#include "log.h"
#include "singleton.h"
#include "utility.h"
#include "macro.h"
#include "fiber.h"

using namespace SingletonSpace;
using namespace LogSpace;
using namespace UtilitySpace;
using namespace MacroSpace;
using namespace FiberSpace;

void run_in_fiber()
{
	shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));

	event->getSstream() << "run_in_fiber begin";
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

	Fiber::YieldTOHold();

	event->setSstream("run_in_fiber end");
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

	Fiber::YieldTOHold();
}

int main(int argc, char** argv) 
{
	shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
	event->setSstream("main begin");
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

	

	Fiber::GetThis();

	event->setSstream("fiber test begin");
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

	shared_ptr<Fiber> fiber(new Fiber(run_in_fiber));

	fiber->swapIn();

	event->setSstream("after swapIn()");
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

	fiber->swapIn();

	event->setSstream("fiber test end");
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

	fiber->swapIn();

	event->setSstream("main end");
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

	return 0;
}
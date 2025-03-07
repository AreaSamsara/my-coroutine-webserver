// #include "common/log.h"
// #include "common/singleton.h"
// #include "common/utility.h"
// #include "concurrent/fiber.h"
// #include "concurrent/scheduler.h"

// using namespace SingletonSpace;
// using namespace LogSpace;
// using namespace FiberSpace;
// using namespace UtilitySpace;
// using namespace ThreadSpace;
// using namespace SchedulerSpace;

// void test_fiber()
// {
//     static int s_count = 5;

//     shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//     event->getSstream() << "test in fiber s_count=" << s_count;
//     Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

//     sleep(1);

//     if (--s_count >= 0)
//     {
//         Scheduler::t_scheduler->schedule(&test_fiber);
//         // Scheduler::t_scheduler->schedule(&test_fiber,GetThread_id());
//     }
// }

// int main(int argc, char **argv)
// {
//     shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//     event->setSstream("main begin");
//     Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

//     // Scheduler sche(3,true,"test");
//     Scheduler sche(3, false, "test");

//     sche.start();

//     sleep(2);

//     event->setSstream("schedule");
//     Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//     sche.schedule(&test_fiber);

//     sche.stop();

//     event->setSstream("main end");
//     Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

//     return 0;
// }
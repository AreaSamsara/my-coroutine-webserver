// #include "common/log.h"
// #include "common/singleton.h"
// #include "common/utility.h"
// #include "concurrent/thread.h"

// using namespace SingletonSpace;
// using namespace LogSpace;
// using namespace ThreadSpace;

// SpinLock s_mutex;
// volatile int count = 0;

// void func1()
// {
//     shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
//     event->getSstream() << " name: " << Thread::getThis()->getName()
//                         << " id: " << UtilitySpace::getThread_id()
//                         << " this.id: " << Thread::getThis()->getId();
//     Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

//     for (int i = 0; i < 100000; ++i)
//     {
//         ScopedLock<SpinLock> lock(s_mutex);
//         // ReadScopedLock<RWMutex> lock(s_mutex);
//         ++count;
//     }
// }

// void func2()
// {
//     while (true)
//     {
//         shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
//         event->setSstream("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
//         Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//     }
// }

// void func3()
// {
//     while (true)
//     {
//         shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
//         event->setSstream("==============================");
//         Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//     }
// }

// int main(int argc, char **argv)
// {
//     // 设置日志事件
//     //__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
//     shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
//     event->getSstream() << "thread test begin";
//     // 使用LoggerManager单例的默认logger输出日志
//     Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

//     vector<shared_ptr<Thread>> threads;
//     for (int i = 0; i < 2; ++i)
//     {
//         shared_ptr<Thread> thread1(new Thread(func2, "name_" + to_string(i)));
//         shared_ptr<Thread> thread2(new Thread(func3, "name_" + to_string(i)));
//         threads.push_back(thread1);
//         threads.push_back(thread2);
//     }

//     for (int i = 0; i < threads.size(); ++i)
//     {
//         threads[i]->join();
//     }

//     event->setSstream("thread test end");
//     Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

//     event->setSstream("");
//     event->getSstream() << "count=" << count;
//     Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

//     return 0;
// }
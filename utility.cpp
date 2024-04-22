#include "utility.h"
#include <sys/syscall.h>
#include "singleton.h"

namespace UtilitySpace
{
	using namespace SingletonSpace;

	//获取当前线程id
	pid_t GetThread_id()
	{
		//用流获取线程在进程内的id
		/*stringstream ss;
		ss << std::this_thread::get_id();
		pid_t thread_id = atoi(ss.str().c_str());
		return thread_id;*/

		//调用系统函数获取线程在系统中的id
		return syscall(SYS_gettid);
	}

	void Backtrace(vector<string>& bt, const int size, const int skip)
	{
		void** array = (void**)malloc((sizeof(void*) * size));
		size_t s = backtrace(array, size);

		char** strings = backtrace_symbols(array, s);
		if (strings == NULL)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
			event->getSstream() << " backtrace_symbols error ";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			return;
		}

		for (size_t i = skip; i < s; ++i)
		{
			bt.push_back(strings[i]);
		}

		free(strings);
		free(array);
	}

	string BacktraceToString(const int size, const int skip, const string & prefix)
	{
		vector<string> bt;
		Backtrace(bt, size, skip);
		stringstream ss;
		for (size_t i = 0; i < bt.size(); ++i)
		{
			ss << prefix << bt[i] << endl;
		}
		return ss.str();
	}

	void Assert(shared_ptr<LogEvent> event)
	{
		event->getSstream() << "Assertion: " << "\nbacktrace:\n" << BacktraceToString(100, 2, "    ");
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		assert(false);
	}

	void Assert(shared_ptr<LogEvent> event, const string& message)
	{
		event->getSstream() << "Assertion: " << "\n" << message << "\nbacktrace:\n" << BacktraceToString(100, 2, "    ");
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		assert(false);
	}
}
#include "utility.h"
#include <sys/syscall.h>
#include <sys/time.h>

#include "singleton.h"
#include "thread.h"
#include "fiber.h"

namespace UtilitySpace
{
	using namespace SingletonSpace;

	//��ȡ��ǰ�߳�id
	pid_t GetThread_id()
	{
		//������ȡ�߳��ڽ����ڵ�id
		/*stringstream ss;
		ss << std::this_thread::get_id();
		pid_t thread_id = atoi(ss.str().c_str());
		return thread_id;*/

		//����ϵͳ������ȡ�߳���ϵͳ�е�id
		return syscall(SYS_gettid);
	}
	//��ȡ��ǰ�߳�����
	string GetThread_name()
	{
		//�����ǰû�����̱߳�����������"main_thread"
		if (ThreadSpace::Thread::t_thread == nullptr)
		{
			return "main_thread";
		}
		//���򷵻ص�ǰ�߳�����
		return ThreadSpace::Thread::t_thread->getName();
	}

	//��ȡ��ǰЭ��id
	uint32_t GetFiber_id()
	{
		return FiberSpace::Fiber::GetFiber_id();
	}

	//ʱ��ms
	uint64_t GetCurrentMS()
	{
		struct timeval time_value;
		gettimeofday(&time_value, NULL);
		return time_value.tv_sec * 1000ul + time_value.tv_usec / 1000;
	}
	//ʱ��us
	uint64_t GetCurrentUS()
	{
		struct timeval time_value;
		gettimeofday(&time_value, NULL);
		return time_value.tv_sec * 1000 * 1000 + time_value.tv_usec;
	}

	void Backtrace(vector<string>& bt, const int size, const int skip)
	{
		void** array = (void**)malloc((sizeof(void*) * size));
		size_t s = backtrace(array, size);

		char** strings = backtrace_symbols(array, s);
		if (strings == NULL)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << " backtrace_symbols error ";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return;
		}

		for (size_t i = skip; i < s; ++i)
		{
			bt.push_back(strings[i]);
		}

		free(strings);
		free(array);
	}

	string BacktraceToString(const int size, const int skip, const string& prefix)
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

	//����ջ������ջ��Ϣ�����жϳ���
	void Assert(shared_ptr<LogEvent> log_event)
	{
		log_event->getSstream() << "Assertion: " << "\nbacktrace:\n" << BacktraceToString(100, 2, "    ");
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
		assert(false);
	}

	//����ջ������ջ��Ϣ��message�ַ��������жϳ���
	void Assert(shared_ptr<LogEvent> log_event, const string& message)
	{
		log_event->getSstream() << "Assertion: " << "\n" << message << "\nbacktrace:\n" << BacktraceToString(100, 2, "    ");
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
		assert(false);
	}
}
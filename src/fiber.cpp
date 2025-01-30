#include "fiber.h"
#include <exception>

#include "common/log.h"
#include "common/singleton.h"
#include "common/utility.h"
#include "scheduler.h"

namespace FiberSpace
{
	using namespace LogSpace;
	using namespace SingletonSpace;
	using namespace UtilitySpace;
	using namespace SchedulerSpace;
	using std::exception;

	// class Fiber:public
	// ���ڴ�����Э�̵Ĺ��캯��
	Fiber::Fiber(const function<void()> &callback, const size_t stacksize)
		: m_id(++s_new_fiber_id), m_stacksize(stacksize), m_callback(callback)
	{
		// ��̬������Э������һ
		++s_fiber_count;

		// ΪЭ��ջ�����ڴ�
		// m_stack = MallocStackAllocator::Allocate(m_stacksize);
		m_stack = malloc(m_stacksize);

		// ��ȡ��ǰ�ﾳ���ɹ��򷵻�0�����򱨴�
		if (getcontext(&m_context) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "getcontext");
		}

		// ����ucontext_t�ṹ��ĳ�Ա����
		m_context.uc_link = nullptr;			  // uc_link��Ϊnullptr�����ڵ�ǰ�����Ľ�����ϵͳ����ֹ
		m_context.uc_stack.ss_sp = m_stack;		  // ��ָ��ջ�׵�ָ��ָ��m_stack����Unixϵͳ�У�ջ�ǴӸߵ�ַ��͵�ַ�����ģ��������ָ��ʵ����ָ�����ջ�Ķ�����
		m_context.uc_stack.ss_size = m_stacksize; // ����ջ�Ĵ�С

		// �����ﾳ,0��ʾ�������κβ���
		makecontext(&m_context, &Fiber::MainFunction, 0);

		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "Fiber::Fiber id=" << m_id;
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_DEBUG, log_event);
	}

	Fiber::~Fiber()
	{
		// ��̬������Э��������һ
		--s_fiber_count;

		// ���Ҫ���ٵ�Э�̵�ջ�ǿգ�˵������Э��
		if (m_stack)
		{
			// ջ�������ٵ�Э�̱��봦����ֹ����ʼ�����쳣��״̬�У����򱨴�
			if (m_state != TERMINAL && m_state != INITIALIZE && m_state != EXCEPT)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				Assert(log_event);
			}

			// �ͷ�ջ�ڴ�
			// MallocStackAllocator::Deallocate(m_stack, m_stacksize);
			free(m_stack);
		}
		// ����˵��Ҫ���ٵ�����Э��
		else
		{
			// ��Э�̲�Ӧ���лص�������Ҳ��Ӧ�ò�����ִ��״̬�����򱨴�
			if (m_callback || m_state != EXECUTE)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				Assert(log_event);
			}

			// �����ǰЭ�����Ǳ�Э�̣�����֮
			if (t_fiber == this)
			{
				t_fiber = nullptr;
			}
		}

		// shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "Fiber::~Fiber id=" << m_id;
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_DEBUG, log_event);
	}

	// ����Э�̺�����״̬��������ȥ�ڴ���ٷ��䣬�ڿ��е��ѷ����ڴ���ֱ�Ӵ�����Э��
	void Fiber::reset(const function<void()> &callback)
	{
		// Ҫ���õ�Э��ջ��ӦΪ�գ�����Э�̲�Ӧ��Ϊ��Э�̣����ұ��봦����ֹ����ʼ�����쳣��״̬�У����򱨴�
		if (!m_stack || (m_state != TERMINAL && m_state != INITIALIZE && m_state != EXCEPT))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		// �������ûص�����
		m_callback = callback;

		// ���»�ȡ��ǰ�ﾳ���ɹ��򷵻�0�����򱨴�
		if (getcontext(&m_context) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "getcontext");
		}

		// ��������ucontext_t�ṹ��ĳ�Ա����
		m_context.uc_link = nullptr;			  // uc_link��Ϊnullptr�����ڵ�ǰ�����Ľ�����ϵͳ����ֹ
		m_context.uc_stack.ss_sp = m_stack;		  // ��ָ��ջ�׵�ָ��ָ��m_stack����Unixϵͳ�У�ջ�ǴӸߵ�ַ��͵�ַ�����ģ��������ָ��ʵ����ָ�����ջ�Ķ�����
		m_context.uc_stack.ss_size = m_stacksize; // ����ջ�Ĵ�С

		// ���������ﾳ,0��ʾ�������κβ���
		makecontext(&m_context, &Fiber::MainFunction, 0);

		// ����״̬Ϊ��ʼ��״̬
		m_state = INITIALIZE;
	}

	// �л�����Э��ִ��
	void Fiber::swapIn()
	{
		// ����ǰЭ���л�Ϊ��Э��
		t_fiber = this;

		// Э��״̬��Ӧ��Ϊִ��״̬�����򱨴�
		if (m_state == EXECUTE)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		// ��Э��״̬����Ϊִ��״̬
		m_state = EXECUTE;

		// �������������Э���ﾳ���л�����Э���ﾳ���ɹ�����0�����򱨴�
		if (swapcontext(&Scheduler::t_scheduler_fiber->m_context, &m_context) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "swapcontext");
		}
	}

	// ����Э���л�����̨
	void Fiber::swapOut()
	{
		// ����ǰЭ������Ϊ����������Э��
		t_fiber = Scheduler::t_scheduler_fiber;

		// ���汾Э���ﾳ���л�����Э���ﾳ���ɹ�����0�����򱨴�
		if (swapcontext(&m_context, &Scheduler::t_scheduler_fiber->m_context) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "swapcontext");
		}
	}

	// class Fiber:public static
	// ��ȡ��ǰЭ�̣������ڵ�һ�ε���ʱ������Э��
	shared_ptr<Fiber> Fiber::GetThis()
	{
		// �����ǰЭ�̲����ڣ�˵���ǵ�һ�ε��ã����ȴ�����Э��
		if (!t_fiber)
		{
			// ����˽�е�Ĭ�Ϲ��캯��������Э��
			shared_ptr<Fiber> main_fiber(new Fiber);

			// ��Э�̴����󣬵�ǰЭ��Ӧ����˽�е�Ĭ�Ϲ��캯���ڱ�����Ϊ��Э�̣����򱨴�
			if (t_fiber != main_fiber.get())
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				Assert(log_event);
			}

			// ���þ�̬������Э��
			t_main_fiber = main_fiber;
		}

		// ���ص�ǰЭ��
		return t_fiber->shared_from_this();
	}

	// ��Э���л�����̨������ΪReady״̬
	void Fiber::YieldToReady()
	{
		// ��ȡ��ǰЭ��
		shared_ptr<Fiber> current = GetThis();
		// ����ǰЭ��״̬����Ϊ׼��״̬
		current->m_state = READY;
		// ����ǰЭ���л�����̨
		current->swapOut();
	}

	// ��Э���л�����̨������ΪHold״̬
	void Fiber::YieldToHold()
	{
		// ��ȡ��ǰЭ��
		shared_ptr<Fiber> current = GetThis();
		// ����ǰЭ��״̬����Ϊ����״̬
		current->m_state = HOLD;
		// ����ǰЭ���л�����̨
		current->swapOut();
	}

	// ��ȡ��ǰЭ��id
	uint64_t Fiber::GetFiber_id()
	{
		// �����ǰЭ�̴��ڣ�������id
		if (t_fiber)
		{
			return t_fiber->getId();
		}
		return 0;
	}

	// Э�̵������к���
	void Fiber::MainFunction()
	{
		// ��ȡ��ǰЭ��
		shared_ptr<Fiber> current = GetThis();
		// ��ǰЭ��Ӧ��Ϊ��Э�̣�����Ӧ��Ϊ��ָ�룬���򱨴�
		if (current == nullptr)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		try
		{
			// ִ�лص�����
			current->m_callback();
			// ִ����ص������������
			current->m_callback = nullptr;
			// ִ����󽫵�ǰЭ��״̬����Ϊ��ֹ״̬
			current->m_state = TERMINAL;
		}
		catch (exception &ex)
		{
			// ����ǰЭ��״̬����Ϊ�쳣״̬
			current->m_state = EXCEPT;

			// shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "Fiber Except: " << ex.what() << " fiber_id=" << current->getId()
									<< "\nbacktrace:\n"
									<< BacktraceToString();
			// ʹ��LoggerManager������Ĭ��logger�����־
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
		}
		catch (...)
		{
			// ����ǰЭ��״̬����Ϊ�쳣״̬
			current->m_state = EXCEPT;

			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "Fiber Except";
			// ʹ��LoggerManager������Ĭ��logger�����־
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
		}

		// ���л�����̨֮ǰ������ָ���л�Ϊ��ָ�룬��ֹshared_ptr�ļ�������������������������
		auto raw_ptr = current.get();
		// ���shared_ptr
		current.reset();
		// ʹ����ָ�뽫��ǰЭ���л�����̨
		raw_ptr->swapOut();

		// ������˵��Զ���ᵽ��˴������򱨴�
		if (true)
		{
			// shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "should never reach!");
		}
	}

	// class Fiber:private
	// Ĭ�Ϲ��캯����˽�з��������ڳ��δ�����Э��ʱ����̬��GetThis()��������
	Fiber::Fiber()
	{
		// ��Э�̳�ʼ״̬��Ϊִ��״̬
		m_state = EXECUTE;
		// ����ǰЭ������Ϊ��Э��
		t_fiber = this;

		// ��ȡ��ǰ�ﾳ���ɹ��򷵻�0�����򱨴�
		if (getcontext(&m_context) != 0)
		{
			// shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "getcontext");
		}

		// ��̬������Э������һ
		++s_fiber_count;

		// shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "Fiber::Fiber";
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_DEBUG, log_event);
	}

	// class Fiber:public static variable
	// ÿ���߳�ר���ĵ�ǰЭ�̣��߳�ר�������������������߳�������������ʹ����ָ�룩
	thread_local Fiber *Fiber::t_fiber = nullptr;
	// ÿ���߳�ר������Э��
	thread_local shared_ptr<Fiber> Fiber::t_main_fiber = nullptr;

	// class Fiber:private static variable
	// ��һ����Э�̵�id����̬ԭ�����ͣ���֤��д���̰߳�ȫ��
	atomic<uint64_t> Fiber::s_new_fiber_id{0};
	// Э����������̬ԭ�����ͣ���֤��д���̰߳�ȫ��
	atomic<uint64_t> Fiber::s_fiber_count{0};
}
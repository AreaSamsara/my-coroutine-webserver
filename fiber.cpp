#include "fiber.h"
#include <exception>

#include "log.h"
#include "singleton.h"
#include "utility.h"
#include "scheduler.h"

namespace FiberSpace
{
	using namespace LogSpace;
	using namespace SingletonSpace;
	using namespace UtilitySpace;
	using namespace SchedulerSpace;
	using std::exception;

	//Ĭ�Ϲ��캯����˽�з��������ڳ��δ�����Э��ʱ����̬��GetThis()��������
	Fiber::Fiber()
	{
		//��Э�̳�ʼ״̬��Ϊִ��״̬
		m_state = EXECUTE;
		//����ǰЭ������Ϊ��Э��
		SetThis(this);

		//��ȡ��ǰ�ﾳ���ɹ��򷵻�0�����򱨴�
		if (getcontext(&m_context) != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event, "getcontext");
		}

		//��̬������Э������һ
		++s_fiber_count;

		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "Fiber::Fiber";
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::DEBUG, event);
	}

	//���ڴ�����Э�̵Ĺ��캯��
	Fiber::Fiber(function<void()> callback, bool use_caller, size_t stacksize)
		:m_id(++s_new_fiber_id), m_stacksize(stacksize), m_callback(callback)
	{
		//��̬������Э������һ
		++s_fiber_count;

		//ΪЭ��ջ�����ڴ�
		m_stack = MallocStackAllocator::Allocate(m_stacksize);

		//��ȡ��ǰ�ﾳ���ɹ��򷵻�0�����򱨴�
		if (getcontext(&m_context) != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event, "getcontext");
		}

		m_context.uc_link = nullptr;
		m_context.uc_stack.ss_sp = m_stack;
		m_context.uc_stack.ss_size = m_stacksize;

		if (!use_caller)
		{
			//�����ﾳ,0��ʾ�������κβ���
			makecontext(&m_context, &Fiber::MainFunction, 0);
		}
		else
		{
			//�����ﾳ,0��ʾ�������κβ���
			//makecontext(&m_context, &Fiber::CallerMainFunction, 0);
			makecontext(&m_context, &Fiber::MainFunction, 0);
		}

		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "Fiber::Fiber id=" << m_id;
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::DEBUG, event); 
	}

	Fiber::~Fiber()
	{
		//��̬������Э��������һ
		--s_fiber_count;

		//���Ҫ���ٵ�Э�̵�ջ�ǿգ�˵������Э��
		if (m_stack)
		{
			//ջ�������ٵ�Э�̱��봦����ֹ����ʼ�����쳣��״̬�У����򱨴�
			if (m_state != TERMINAL && m_state != INITIALIZE && m_state != EXCEPT)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}
			
			MallocStackAllocator::Deallocate(m_stack, m_stacksize);
		}
		//����Ҫ���ٵ�����Э��
		else
		{
			//��Э�̲�Ӧ���лص�������Ҳ��Ӧ�ò�����ִ��״̬�����򱨴�
			if (m_callback || m_state != EXECUTE)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}

			Fiber* current = t_fiber;
			//�����ǰЭ�����Ǳ�Э�̣�����֮
			if (current == this)
			{
				SetThis(nullptr);
			}
		}

		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "Fiber::~Fiber id=" << m_id;
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::DEBUG, event);
	}

	//����Э�̺�����״̬
	//INIT��TERM
	void Fiber::reset(function<void()> callback)
	{
		//Ҫ���õ�Э��ջ��ӦΪ�գ���Ӧ��Ϊ��Э�̣����ұ��봦����ֹ����ʼ�����쳣��״̬�У����򱨴�
		if (!m_stack || (m_state != TERMINAL && m_state != INITIALIZE && m_state != EXCEPT))
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//�������ûص�����
		m_callback = callback;

		//��ȡ��ǰ�ﾳ���ɹ��򷵻�0�����򱨴�
		if (getcontext(&m_context) != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event, "getcontext");
		}

		m_context.uc_link = nullptr;
		m_context.uc_stack.ss_sp = m_stack;
		m_context.uc_stack.ss_size = m_stacksize;

		//�����ﾳ,0��ʾ�������κβ���
		makecontext(&m_context, &Fiber::MainFunction, 0);

		//����״̬Ϊ��ʼ��״̬
		m_state = INITIALIZE;
	}

	//�л�����Э��ִ��
	void Fiber::swapIn()
	{
		//����ǰЭ���л�Ϊ��Э��
		SetThis(this);

		//Э��״̬��Ӧ��Ϊִ��״̬�����򱨴�
		if (m_state == EXECUTE)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//��״̬����Ϊִ��״̬
		m_state = EXECUTE;

		//�������������Э���ﾳ���л�����Э���ﾳ���ɹ�����0�����򱨴�
		if (swapcontext(&Scheduler::GetMainFiber()->m_context, &m_context) != 0)

		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event, "swapcontext");
		}
	}

	//����Э���л�����̨
	void Fiber::swapOut()
	{
		//����ǰЭ������Ϊ����������Э��
		SetThis(Scheduler::GetMainFiber());

		//���汾Э���ﾳ���л�����Э���ﾳ���ɹ�����0�����򱨴�
		if (swapcontext(&m_context, &Scheduler::GetMainFiber()->m_context) != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event, "swapcontext");
		}	
	}

	void Fiber::call()
	{
		//����ǰЭ���л�Ϊ��Э��
		SetThis(this);

		//Э��״̬��Ӧ��Ϊִ��״̬�����򱨴�
		if (m_state == EXECUTE)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//��״̬����Ϊִ��״̬
		m_state = EXECUTE;

		//�������������Э���ﾳ���л�����Э���ﾳ���ɹ�����0�����򱨴�
		if (swapcontext(&t_main_fiber->m_context, &m_context) != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event, "swapcontext");
		}
	}

	void Fiber::back()
	{
		//����ǰЭ������Ϊ����������Э��
		SetThis(t_main_fiber.get());

		//���汾Э���ﾳ���л�����Э���ﾳ���ɹ�����0�����򱨴�
		if (swapcontext(&m_context, &t_main_fiber->m_context) != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event, "swapcontext");
		}
	}


	//���õ�ǰЭ��Ϊfiber
	void Fiber::SetThis(Fiber* fiber)
	{
		t_fiber = fiber;
	}

	//��ȡ��ǰЭ��
	shared_ptr<Fiber> Fiber::GetThis()
	{
		//�����ǰЭ�̲����ڣ�˵���ǵ�һ�ε��ã����ȴ�����Э��
		if (!t_fiber)
		{
			//����˽�е�Ĭ�Ϲ��캯��������Э��
			shared_ptr<Fiber> main_fiber(new Fiber);

			//��Э�̴����󣬵�ǰЭ��Ӧ��������Ϊ��Э�̣����򱨴�
			if (t_fiber != main_fiber.get())
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}

			//���þ�̬������Э��
			t_main_fiber = main_fiber;
		}

		//���ص�ǰЭ��
		return t_fiber->shared_from_this();
	}

	//Э���л�����̨������ΪReady״̬
	void Fiber::YieldTOReady()
	{
		//��ȡ��ǰЭ��
		shared_ptr<Fiber> current = GetThis();
		//����ǰЭ��״̬����Ϊ׼��״̬
		current->m_state = READY;
		//����ǰЭ���л�����̨
		current->swapOut();
	}

	//Э���л�����̨������ΪHold״̬
	void Fiber::YieldTOHold()
	{
		//��ȡ��ǰЭ��
		shared_ptr<Fiber> current = GetThis();
		//����ǰЭ��״̬����Ϊ����״̬
		current->m_state = HOLD;
		//����ǰЭ���л�����̨
		current->swapOut();
	}

	//��ȡ��Э����
	uint64_t Fiber::GetFiber_count()
	{
		return s_fiber_count;
	}

	//��ȡ��ǰЭ��id
	uint64_t Fiber::GetFiber_id()
	{
		//�����ǰЭ�̴��ڣ�������id
		if (t_fiber)
		{
			return t_fiber->getId();
		}
		return 0;
	}

	//Э�̵������к���
	void Fiber::MainFunction()
	{
		shared_ptr<Fiber> current = GetThis();
		//��ǰЭ��Ӧ��Ϊ��Э�̣�����Ӧ��Ϊ��ָ�룬���򱨴�
		if (current == nullptr)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}
		
		try
		{
			//ִ�лص�����
			current->m_callback();
			//ִ����ص������������
			current->m_callback = nullptr;
			//����ǰЭ��״̬����Ϊ��ֹ״̬
			current->m_state = TERMINAL;
		}
		catch(exception& ex)
		{
			//����ǰЭ��״̬����Ϊ�쳣״̬
			current->m_state = EXCEPT;

			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "Fiber Except: " << ex.what() << " fiber_id=" << current->getId()
				<< "\nbacktrace:\n" << BacktraceToString();
			//ʹ��LoggerManager������Ĭ��logger�����־
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		}
		catch (...)
		{
			//����ǰЭ��״̬����Ϊ�쳣״̬
			current->m_state = EXCEPT;

			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "Fiber Except";
			//ʹ��LoggerManager������Ĭ��logger�����־
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		}

		//���л�����̨֮ǰ������ָ���л�Ϊ��ָ�룬��ֹshared_ptr�ļ�������������������������
		auto raw_ptr = current.get();
		//���shared_ptr
		current.reset();	
		//ʹ����ָ�뽫��ǰЭ���л�����̨
		raw_ptr->swapOut();

		//������˵��Զ���ᵽ��˴������򱨴�
		if (true)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event, "should never reach!");
		}
	}

	void Fiber::CallerMainFunction()
	{
		shared_ptr<Fiber> current = GetThis();
		//��ǰЭ��Ӧ��Ϊ��Э�̣�����Ӧ��Ϊ��ָ�룬���򱨴�
		if (current == nullptr)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		try
		{
			//ִ�лص�����
			current->m_callback();
			//ִ����ص������������
			current->m_callback = nullptr;
			//����ǰЭ��״̬����Ϊ��ֹ״̬
			current->m_state = TERMINAL;
		}
		catch (exception& ex)
		{
			//����ǰЭ��״̬����Ϊ�쳣״̬
			current->m_state = EXCEPT;

			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "Fiber Except: " << ex.what() << " fiber_id=" << current->getId()
				<< "\nbacktrace:\n" << BacktraceToString();
			//ʹ��LoggerManager������Ĭ��logger�����־
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		}
		catch (...)
		{
			//����ǰЭ��״̬����Ϊ�쳣״̬
			current->m_state = EXCEPT;

			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << "Fiber Except";
			//ʹ��LoggerManager������Ĭ��logger�����־
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		}

		//���л�����̨֮ǰ������ָ���л�Ϊ��ָ�룬��ֹshared_ptr�ļ�������������������������
		auto raw_ptr = current.get();
		//���shared_ptr
		current.reset();
		//ʹ����ָ�뽫��ǰЭ���л�����̨
		raw_ptr->back();

		//������˵��Զ���ᵽ��˴������򱨴�
		if (true)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event, "should never reach!");
		}
	}

	//��һ����Э�̵�id����̬ԭ�����ͣ���֤��д���̰߳�ȫ��
	atomic<uint64_t> Fiber::s_new_fiber_id{ 0 };
	//Э����������̬ԭ�����ͣ���֤��д���̰߳�ȫ��
	atomic<uint64_t>  Fiber::s_fiber_count{ 0 };

	//ÿ���߳�ר���ĵ�ǰЭ�̣��߳�ר�������������������߳�����������ʹ����ָ�룩
	thread_local Fiber* Fiber::t_fiber = nullptr;
	//ÿ���߳�ר������Э��
	thread_local shared_ptr <Fiber>  Fiber::t_main_fiber = nullptr;
}
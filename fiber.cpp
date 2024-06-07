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

	//class Fiber:public
	//用于创建子协程的构造函数
	Fiber::Fiber(const function<void()>& callback, const size_t stacksize)
		:m_id(++s_new_fiber_id), m_stacksize(stacksize), m_callback(callback)
	{
		//静态变量：协程数加一
		++s_fiber_count;

		//为协程栈分配内存
		//m_stack = MallocStackAllocator::Allocate(m_stacksize);
		m_stack = malloc(m_stacksize);

		//获取当前语境，成功则返回0，否则报错
		if (getcontext(&m_context) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "getcontext");
		}

		//设置ucontext_t结构体的成员变量
		m_context.uc_link = nullptr;		//uc_link设为nullptr，则在当前上下文结束后，系统将终止
		m_context.uc_stack.ss_sp = m_stack;		//将指向栈底的指针指向m_stack（在Unix系统中，栈是从高地址向低地址增长的，所以这个指针实际上指向的是栈的顶部）
		m_context.uc_stack.ss_size = m_stacksize;	//设置栈的大小

		//设置语境,0表示不传递任何参数
		makecontext(&m_context, &Fiber::MainFunction, 0);

		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "Fiber::Fiber id=" << m_id;
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event); 
	}

	Fiber::~Fiber()
	{
		//静态变量：协程数量减一
		--s_fiber_count;

		//如果要销毁的协程的栈非空，说明是子协程
		if (m_stack)
		{
			//栈将被销毁的协程必须处于终止、初始化或异常的状态中，否则报错
			if (m_state != TERMINAL && m_state != INITIALIZE && m_state != EXCEPT)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				Assert(log_event);
			}
			
			//释放栈内存
			//MallocStackAllocator::Deallocate(m_stack, m_stacksize);
			free(m_stack);
		}
		//否则说明要销毁的是主协程
		else
		{
			//主协程不应具有回调函数，也不应该不处于执行状态，否则报错
			if (m_callback || m_state != EXECUTE)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				Assert(log_event);
			}

			//如果当前协程正是本协程，销毁之
			if(t_fiber == this)
			{
				t_fiber = nullptr;
			}
		}

		//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "Fiber::~Fiber id=" << m_id;
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);
	}

	//重置协程函数和状态，用于免去内存的再分配，在空闲的已分配内存上直接创建新协程
	void Fiber::reset(const function<void()>& callback)
	{
		//要重置的协程栈不应为空（即该协程不应该为主协程），且必须处于终止、初始化或异常的状态中，否则报错
		if (!m_stack || (m_state != TERMINAL && m_state != INITIALIZE && m_state != EXCEPT))
		{
			//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		//重新设置回调函数
		m_callback = callback;

		//重新获取当前语境，成功则返回0，否则报错
		if (getcontext(&m_context) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "getcontext");
		}

		//重新设置ucontext_t结构体的成员变量
		m_context.uc_link = nullptr;		//uc_link设为nullptr，则在当前上下文结束后，系统将终止
		m_context.uc_stack.ss_sp = m_stack;		//将指向栈底的指针指向m_stack（在Unix系统中，栈是从高地址向低地址增长的，所以这个指针实际上指向的是栈的顶部）
		m_context.uc_stack.ss_size = m_stacksize;	//设置栈的大小

		//重新设置语境,0表示不传递任何参数
		makecontext(&m_context, &Fiber::MainFunction, 0);

		//重设状态为初始化状态
		m_state = INITIALIZE;
	}

	//切换到本协程执行
	void Fiber::swapIn()
	{
		//将当前协程切换为本协程
		t_fiber = this;

		//协程状态不应该为执行状态，否则报错
		if (m_state == EXECUTE)
		{
			//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		//将协程状态设置为执行状态
		m_state = EXECUTE;

		//保存调度器的主协程语境，切换到本协程语境，成功返回0，否则报错
		if (swapcontext(&Scheduler::t_scheduler_fiber->m_context, &m_context) != 0)
		{
			//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "swapcontext");
		}
	}

	//将本协程切换到后台
	void Fiber::swapOut()
	{
		//将当前协程设置为调度器的主协程
		t_fiber = Scheduler::t_scheduler_fiber;

		//保存本协程语境，切换到主协程语境，成功返回0，否则报错
		if (swapcontext(&m_context, &Scheduler::t_scheduler_fiber->m_context) != 0)
		{
			//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "swapcontext");
		}	
	}





	//class Fiber:public static
	//获取当前协程，并仅在第一次调用时创建主协程
	shared_ptr<Fiber> Fiber::GetThis()
	{
		//如果当前协程不存在，说明是第一次调用，则先创建主协程
		if (!t_fiber)
		{
			//调用私有的默认构造函数创建主协程
			shared_ptr<Fiber> main_fiber(new Fiber);

			//主协程创建后，当前协程应当在私有的默认构造函数内被设置为主协程，否则报错
			if (t_fiber != main_fiber.get())
			{
				//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				Assert(log_event);
			}

			//设置静态变量主协程
			t_main_fiber = main_fiber;
		}

		//返回当前协程
		return t_fiber->shared_from_this();
	}

	//将协程切换到后台并设置为Ready状态
	void Fiber::YieldToReady()
	{
		//获取当前协程
		shared_ptr<Fiber> current = GetThis();
		//将当前协程状态设置为准备状态
		current->m_state = READY;
		//将当前协程切换到后台
		current->swapOut();
	}

	//将协程切换到后台并设置为Hold状态
	void Fiber::YieldToHold()
	{
		//获取当前协程
		shared_ptr<Fiber> current = GetThis();
		//将当前协程状态设置为挂起状态
		current->m_state = HOLD;
		//将当前协程切换到后台
		current->swapOut();
	}

	//获取当前协程id
	uint64_t Fiber::GetFiber_id()
	{
		//如果当前协程存在，返回其id
		if (t_fiber)
		{
			return t_fiber->getId();
		}
		return 0;
	}

	//协程的主运行函数
	void Fiber::MainFunction()
	{
		//获取当前协程
		shared_ptr<Fiber> current = GetThis();
		//当前协程应该为子协程，而不应当为空指针，否则报错
		if (current == nullptr)
		{
			//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}
		
		try
		{
			//执行回调函数
			current->m_callback();
			//执行完回调函数后将其清空
			current->m_callback = nullptr;
			//执行完后将当前协程状态设置为终止状态
			current->m_state = TERMINAL;
		}
		catch(exception& ex)
		{
			//将当前协程状态设置为异常状态
			current->m_state = EXCEPT;

			//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "Fiber Except: " << ex.what() << " fiber_id=" << current->getId()
				<< "\nbacktrace:\n" << BacktraceToString();
			//使用LoggerManager单例的默认logger输出日志
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
		}
		catch (...)
		{
			//将当前协程状态设置为异常状态
			current->m_state = EXCEPT;

			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "Fiber Except";
			//使用LoggerManager单例的默认logger输出日志
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
		}

		//在切换到后台之前将智能指针切换为裸指针，防止shared_ptr的计数错误导致析构函数不被调用
		auto raw_ptr = current.get();
		//清空shared_ptr
		current.reset();	
		//使用裸指针将当前协程切换到后台
		raw_ptr->swapOut();

		//按理来说永远不会到达此处，否则报错
		if (true)
		{
			//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "should never reach!");
		}
	}


	//class Fiber:private
	//默认构造函数，私有方法，仅在初次创建主协程时被静态的GetThis()方法调用
	Fiber::Fiber()
	{
		//主协程初始状态即为执行状态
		m_state = EXECUTE;
		//将当前协程设置为主协程
		t_fiber = this;

		//获取当前语境，成功则返回0，否则报错
		if (getcontext(&m_context) != 0)
		{
			//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event, "getcontext");
		}

		//静态变量：协程数加一
		++s_fiber_count;

		//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "Fiber::Fiber";
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);
	}



	//class Fiber:public static variable
	//每个线程专属的当前协程（线程专属变量的生命周期由线程自主管理，故使用裸指针）
	thread_local Fiber* Fiber::t_fiber = nullptr;
	//每个线程专属的主协程
	thread_local shared_ptr <Fiber>  Fiber::t_main_fiber = nullptr;


	//class Fiber:private static variable
	//下一个新协程的id（静态原子类型，保证读写的线程安全）
	atomic<uint64_t> Fiber::s_new_fiber_id{ 0 };
	//协程总数（静态原子类型，保证读写的线程安全）
	atomic<uint64_t>  Fiber::s_fiber_count{ 0 };
}
#include "fiber.h"
#include <atomic>
#include <exception>

#include "macro.h"
#include "log.h"
#include "singleton.h"

namespace FiberSpace
{
	using std::atomic;
	using std::exception;
	using namespace MacroSpace;
	using namespace LogSpace;
	using namespace SingletonSpace;

	static atomic<uint64_t> s_fiber_id{ 0 };
	static atomic<uint64_t> s_fiber_count{ 0 };

	static thread_local Fiber* t_fiber = nullptr;
	static thread_local shared_ptr <Fiber> t_threadFiber = nullptr;

	Fiber::Fiber()
	{
		m_state = EXEC;
		SetThis(this);

		if (getcontext(&m_context) != 0)
		{
			Assert(false, "getcontext");
		}

		++s_fiber_count;

		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
		event->getSstream() << "Fiber::Fiber";
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::DEBUG, event);
	}

	Fiber::Fiber(function<void()> callback, size_t stacksize)
		:m_id(++s_fiber_id),m_callback(callback)
	{
		++s_fiber_count;
		m_stacksize = stacksize;

		m_stack = MallocStackAllocator::Allocate(m_stacksize);
		if (getcontext(&m_context))
		{
			Assert(false, "getcontext");
		}
		m_context.uc_link = nullptr;
		m_context.uc_stack.ss_sp = m_stack;
		m_context.uc_stack.ss_size = m_stacksize;

		makecontext(&m_context, &Fiber::MainFunc, 0);

		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
		event->getSstream() << "Fiber::Fiber id=" << m_id;
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::DEBUG, event); 
	}

	Fiber::~Fiber()
	{
		--s_fiber_count;
		if (m_stack)
		{
			Assert(m_state == TERM || m_state == INIT || m_state == EXCEPT);
			MallocStackAllocator::Deallocate(m_stack, m_stacksize);
		}
		else
		{
			Assert(!m_callback);
			Assert(m_state == EXEC);

			Fiber* current = t_fiber;
			if (current == this)
			{
				SetThis(nullptr);
			}
		}

		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
		event->getSstream() << "Fiber::~Fiber id=" << m_id;
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
	}

	//重置协程函数和状态
	//INIT，TERM
	void Fiber::reset(function<void()> callback)
	{
		Assert(m_stack);
		Assert(m_state == TERM || m_state == INIT || m_state == EXCEPT);

		m_callback = callback;
		if (getcontext(&m_context))
		{
			Assert(false, "getcontext");
		}

		m_context.uc_link = nullptr;
		m_context.uc_stack.ss_sp = m_stack;
		m_context.uc_stack.ss_size = m_stacksize;

		makecontext(&m_context, &Fiber::MainFunc, 0);
		m_state = INIT;
	}

	//切换到当前协程执行
	void Fiber::swapIn()
	{
		SetThis(this);
		Assert(m_state != EXEC);
		m_state = EXEC;

		if (swapcontext(&t_threadFiber->m_context,&m_context))
		{
			cout << "adadadad";
			Assert(false, "swapcontext");
		}
	}

	//切换到后台执行
	void Fiber::swapOut()
	{
		SetThis(t_threadFiber.get());
		if (swapcontext(&m_context, &t_threadFiber->m_context))
		{
			Assert(false, "swapcontext");
		}
	}

	//设置当前协程
	void Fiber::SetThis(Fiber* fiber)
	{
		t_fiber = fiber;
	}

	//获取当前协程
	shared_ptr<Fiber> Fiber::GetThis()
	{
		if (t_fiber)
		{
			return t_fiber->shared_from_this();
		}
		shared_ptr<Fiber> main_fiber(new Fiber);
		Assert(t_fiber == main_fiber.get());
		t_threadFiber = main_fiber;
		return t_fiber->shared_from_this();
	}

	//协程切换到后台并设置为Ready状态
	void Fiber::YieldTOReady()
	{
		shared_ptr<Fiber> current = GetThis();
		current->m_state = READY;
		current->swapOut();
	}

	//协程切换到后台并设置为Hold状态
	void Fiber::YieldTOHold()
	{
		shared_ptr<Fiber> current = GetThis();
		current->m_state = HOLD;
		current->swapOut();
	}

	//总协程数
	uint64_t Fiber::TotalFibers()
	{
		return s_fiber_count;
	}

	void Fiber::MainFunc()
	{
		shared_ptr<Fiber> current = GetThis();
		Assert(current!=nullptr);
		try
		{
			current->m_callback();
			current->m_callback = nullptr;
			current->m_state = TERM;
		}
		catch(exception& ex)
		{
			current->m_state = EXCEPT;
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
			event->getSstream() << "Fiber Except: " << ex.what();
			//使用LoggerManager单例的默认logger输出日志
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		}
		catch (...)
		{
			current->m_state = EXCEPT;
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
			event->getSstream() << "Fiber Except";
			//使用LoggerManager单例的默认logger输出日志
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		}

		//在切换到后台之前将智能指针切换为裸指针，防止shared_ptr的计数错误导致析构函数不被调用
		auto raw_ptr = current.get();
		current.reset();	//清空shared_ptr
		raw_ptr->swapOut();

		//按理来说永远不会到达此处
		Assert(false, "should never reach");
	}

	//获取当前协程id
	uint64_t Fiber::GetFiber_id()
	{
		if (t_fiber)
		{
			return t_fiber->getId();
		}
		return 0;
	}
}
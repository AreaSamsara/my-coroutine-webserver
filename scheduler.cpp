#include "scheduler.h"
#include "log.h"

namespace SchedulerSpace
{
	using std::bind;

	//当前调度器（线程专属）
	static thread_local Scheduler* t_scheduler = nullptr;
	//当前调度器的主协程（线程专属）
	static thread_local Fiber* t_scheduler_fiber = nullptr;

	//使用调用者线程时usecaller为true，否则为false
	Scheduler::Scheduler(size_t thread_count, const bool use_caller, const string& name)
		:m_name(name)
	{
		//线程数应该为正数，否则报错
		if (thread_count <= 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//如果使用调用者线程
		if (use_caller)
		{
			//获取调用者线程
			Fiber::GetThis();
			//调用者线程也计入线程数，故线程池大小应该减一
			--thread_count;


			if (GetThis() != nullptr)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}

			//将线程当前调度器设置为本调度器
			t_scheduler = this;

			//设置调度器的主协程
			m_main_fiber.reset(new Fiber(bind(&Scheduler::run, this),true));
			//将线程调度器主协程设置为调度器的主协程
			t_scheduler_fiber = m_main_fiber.get();

			//设置调度器主线程id为当前线程（调用者线程）的id
			m_main_thread_id = GetThread_id();
			//将调度器主线程id加入调度器id集合
			m_thread_ids.push_back(m_main_thread_id);
		}
		//如果不使用调用者线程
		else
		{
			//将调度器主线程id设置为无效值（-1）
			m_main_thread_id = -1;
		}

		//设置线程数
		m_thread_count = thread_count;
	}

	Scheduler::~Scheduler()
	{
		if (!m_stopping)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}
		if (GetThis() == this)
		{
			t_scheduler = nullptr;
		}
	}

	//启动调度器
	void Scheduler::start()
	{
		//先监视互斥锁，保护
		ScopedLock<Mutex> lock(m_mutex);

		//如果不是正处于停止状态，则不应该继续执行start函数
		if (!m_stopping)
		{
			return;
		}
		//将停止状态关闭
		m_stopping = false;

		//线程池应为空，否则报错
		if (!m_threads.empty())
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//将线程池大小设为线程数
		m_threads.resize(m_thread_count);
		for (size_t i = 0; i < m_thread_count; ++i)
		{
			//创建线程并将其放入线程池
			m_threads[i].reset(new Thread(bind(&Scheduler::run, this), m_name + "_" + to_string(i)));
			//记录池内线程id
			m_thread_ids.push_back(m_threads[i]->getId());	//由于线程构造函数加了信号量，所以此处一定能拿到正确的线程id
		}
	}

	//停止调度器
	void Scheduler::stop()
	{
		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "stop";
		//使用LoggerManager单例的默认logger输出日志
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

		m_autoStop = true;

		if (m_main_fiber && m_thread_count == 0
			&& (m_main_fiber->getState() == Fiber::TERMINAL || m_main_fiber->getState() == Fiber::INITIALIZE))
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << this << "stopped";
			//使用LoggerManager单例的默认logger输出日志
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
		
			m_stopping = true;

			if (stopping())
			{
				return;
			}
		}

		//如果使用了调用者线程
		if (m_main_thread_id != -1)
		{
			if (GetThis() != this)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}
		}
		//如果未使用调用者线程
		else
		{
			if (GetThis() == this)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}
		}
		
		m_stopping = true;

		for (size_t i = 0; i < m_thread_count; ++i)
		{
			tickle();
		}

		//如果使用了调用者线程
		if (m_main_fiber)
		{
			tickle();
		}

		//如果使用了调用者线程
		if (m_main_fiber)
		{
			if (!stopping())
			{
				m_main_fiber->call();
			}
		}

		vector<shared_ptr<Thread>> threads;

		{
			//先监视互斥锁，保护m_threads
			ScopedLock<Mutex> lock(m_mutex);
			//将线程池复制到局部变量再运行，确保m_threads的自由
			threads.swap(m_threads);
		}

		//阻塞正在运行的线程，将线程池内的所有线程加入等待队列
		for (auto& i : threads)
		{
			i->join();
		}
	}

	//通知调度器有任务了
	void Scheduler::tickle()
	{
		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "tickle";
		//使用LoggerManager单例的默认logger输出日志
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
	}

	//调度器分配给线程池内线程的回调函数
	void Scheduler::run()
	{
		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "run" ;
		//使用LoggerManager单例的默认logger输出日志
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::DEBUG, event);

		//先将线程专属的当前调度器设置为本调度器
		setThis();

		//如果当前线程id不等于主线程（调用者线程）id,说明不论是否使用了调用者线程，至少这个线程不是调用者线程
		//将线程的调度器主协程设置为当前线程的当前协程
		if (GetThread_id() != m_main_thread_id)
		{
			t_scheduler_fiber = Fiber::GetThis().get();
		}

		//空闲协程
		shared_ptr<Fiber> idle_fiber(new Fiber(bind(&Scheduler::idle, this)));
		//由回调函数创建的协程
		shared_ptr<Fiber> callback_fiber;

		//任务容器
		Fiber_and_Thread fiber_and_thread;

		//任务执行循环
		while (true)
		{
			//重置任务容器
			fiber_and_thread.reset();
			//是否要通知其他线程来完成任务
			bool tickle_me = false;
			//是否取到了任务
			bool is_active = false;

			//从任务队列里面取出一个应该执行的任务
			{
				//先监视互斥锁，保护
				ScopedLock<Mutex> lock(m_mutex);

				//尝试取得任务
				auto iterator = m_fibers.begin();
				while(iterator != m_fibers.end())
				{
					//该任务已设置线程id，且与当前线程id不同，则不做这个任务，而是通知其他线程来完成
					//反之如果未设置线程id（-1），则所有线程都有义务做这个任务
					if(iterator->m_thread_id!=-1 && iterator->m_thread_id!=GetThread_id())
					{
						++iterator;
						//通知其他线程来完成
						tickle_me = true;
						continue;
					}

					//该任务应包含协程对象或回调函数，否则报错
					if (!iterator->m_fiber && !iterator->m_callback)
					{
						shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
						Assert(event);
					}

					//如果该任务包含的是协程对象，且该协程是执行状态，则不做这个任务
					if (iterator->m_fiber && iterator->m_fiber->getState() == Fiber::EXECUTE)
					{
						++iterator;
						continue;
					}

					//否则该任务可以做，取出该任务
					fiber_and_thread = *iterator;
					//将该任务从任务队列删除，并将迭代器移动一位
					m_fibers.erase(iterator++);
					//在拿出任务以后应立即让活跃的线程数量加一
					++m_active_thread_count;

					//表示已经取到了任务
					is_active = true;

					break;
				}
			}

			//如果任务应该由其他线程完成，则调用tickle()
			if (tickle_me)
			{
				tickle();
			}

			//如果有任务要做且任务包含的是协程且该协程不是终止状态或异常状态，则切换到该协程执行
			if (fiber_and_thread.m_fiber && 
				(fiber_and_thread.m_fiber->getState() != Fiber::TERMINAL
				&& fiber_and_thread.m_fiber->getState() != Fiber::EXCEPT))
			{
				//切换到该协程执行
				fiber_and_thread.m_fiber->swapIn();
				//活跃的线程数量减一
				--m_active_thread_count;

				//如果此时该协程为准备状态，再将其放入任务队列
				if (fiber_and_thread.m_fiber->getState() == Fiber::READY)
				{
					schedule(fiber_and_thread.m_fiber);
				}
				//否则如果该协程不是终止或异常状态，则将其设为挂起状态
				else if (fiber_and_thread.m_fiber->getState() != Fiber::TERMINAL
					&& fiber_and_thread.m_fiber->getState() != Fiber::EXCEPT)
				{
					fiber_and_thread.m_fiber->setState(Fiber::HOLD);
				}

				//本次任务容器的功能已完成，将其重置
				fiber_and_thread.reset();
			}
			//如果有任务要做且任务包含的是回调函数，为其创建一个协程，再切换到该协程执行
			else if (fiber_and_thread.m_callback)
			{
				//如果callback_fiber不为空，调用Fiber类的reset方法重设之
				if (callback_fiber)
				{
					callback_fiber->reset(fiber_and_thread.m_callback);
				}
				//如果callback_fiber为空，调用shared_ptr类的reset方法重设之
				else
				{
					callback_fiber.reset(new Fiber(fiber_and_thread.m_callback));
				}

				//本次任务容器的功能已完成，将其重置
				fiber_and_thread.reset();
				
			
				//切换到该协程执行
				callback_fiber->swapIn();
				//活跃的线程数量减一
				--m_active_thread_count;

				//如果此时该协程为准备状态，再将其放入任务队列
				if (callback_fiber->getState() == Fiber::READY)
				{
					schedule(callback_fiber);
					callback_fiber.reset();
				}
				//否则如果该协程是终止或异常状态，则将其清除
				else if (callback_fiber->getState() == Fiber::EXCEPT
					|| callback_fiber->getState() == Fiber::TERMINAL)
				{
					callback_fiber->reset(nullptr);
				}
				//如果该协程不是准备、终止或异常状态，则将其设为挂起状态
				else
				{
					callback_fiber->setState(Fiber::HOLD);
					callback_fiber.reset();
				}
			}
			//如果没有做任何任务
			else
			{
				//如果取到了任务，说明单纯是这个任务既不包含协程也不包含回调函数，是个空任务（摸会鱼）
				if (is_active)
				{
					//活跃的线程数量减一
					--m_active_thread_count;
					//continue;
				}
				//否则说明任务队列真的没有任务了，执行空闲协程
				else
				{
					//如果空闲协程也已经处于终止状态，则所有任务都完成了，退出循环
					if (idle_fiber->getState() == Fiber::TERMINAL)
					{
						shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
						event->getSstream() << "idle_fiber terminated";
						//使用LoggerManager单例的默认logger输出日志
						Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

						break;
					}
					//否则执行空闲协程
					else
					{
						//活跃的线程数量加一
						++m_idle_thread_count;
						//切换到空闲协程执行
						idle_fiber->swapIn();
						//活跃的线程数量减一
						--m_idle_thread_count;

						//否则如果该协程不是终止或异常状态，则将其设为挂起状态
						if (idle_fiber->getState() != Fiber::TERMINAL
							&& idle_fiber->getState() != Fiber::EXCEPT)
						{
							idle_fiber->setState(Fiber::HOLD);
						}
					}
				}
			}
		}
	}

	//返回是否可以停止
	bool Scheduler::stopping()
	{
		//先监视互斥锁，保护
		ScopedLock<Mutex> lock(m_mutex);
		return m_autoStop && m_stopping && m_fibers.empty() && m_active_thread_count == 0;
	}

	void Scheduler::idle()
	{
		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "idle";
		//使用LoggerManager单例的默认logger输出日志
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

		//在可以停止之前不终止空闲协程而是将其挂起
		while (!stopping())
		{
			Fiber::YieldTOHold();
		}
	}

	//设置当前调度器为本调度器（线程专属）
	void Scheduler::setThis()
	{
		t_scheduler = this;
	}

	//获取当前调度器（线程专属）
	Scheduler* Scheduler::GetThis()
	{
		return t_scheduler;
	}
	//获取当前调度器的主协程（线程专属）
	Fiber* Scheduler::GetMainFiber()
	{
		return t_scheduler_fiber;
	}
}
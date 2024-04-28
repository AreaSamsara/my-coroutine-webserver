#include "scheduler.h"
#include "log.h"

namespace SchedulerSpace
{
	using std::bind;

	//thread_count需要在构造函数内部再确定，故不加const；使用调用者线程时usecaller为true，否则为false
	Scheduler::Scheduler(size_t thread_count, const bool use_caller, const string& name)
		:m_use_caller(use_caller), m_name(name)
	{
		//线程数应该为正数，否则报错
		if (thread_count <= 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//如果使用调用者线程
		if (m_use_caller)
		{
			//如果调用者的主协程不存在，创建之
			//Fiber::GetThis();

			//调用者线程也计入线程数，故线程池大小应该减一
			--thread_count;

			////线程当前调度器应该为空，否则报错
			//if (GetThis() != nullptr)
			//{
			//	shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			//	Assert(event);
			//}

			//将线程当前调度器设置为本调度器
			//t_scheduler = this;

			//为调用者线程创建主协程并将其设为调度器的主协程
			t_scheduler_fiber = Fiber::GetThis().get();

			//设置调用者线程的线程替代者协程(调用者线程有自己的任务，故应当创建协程来执行Scheduler::run()作为替代)
			//m_thread_substitude_caller_fiber.reset(new Fiber(bind(&Scheduler::run, this),true));
			m_thread_substitude_caller_fiber.reset(new Fiber(bind(&Scheduler::run, this)));
			//将线程调度器主协程设置为调用者线程的m_caller_fiber(而不是调用者线程的主协程，m_caller_fiber同时用作调度器的主协程，便于子协程的切换)
			//t_scheduler_fiber = m_caller_fiber.get();

			//设置调用者线程的id
			m_caller_thread_id = GetThread_id();
			//将调用者线程的id加入调度器线程id集合
			m_thread_ids.push_back(m_caller_thread_id);
		}
		//如果不使用调用者线程
		else
		{
			//将调用者线程的id设置为无效值（-1）
			m_caller_thread_id = -1;
		}

		//设置线程数（放在构造函数的末尾以保证准确性）
		m_thread_count = thread_count;
	}

	Scheduler::~Scheduler()
	{
		//既然要析构调度器，那调度器应该处于停止状态，否则报错
		if (!m_stopping)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}
		//如果线程的当前调度器为本调度器，将其置空
		if (t_scheduler == this)
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
			//创建以Scheduler::run为回调函数的线程并将其放入线程池
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
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::DEBUG, event);

		m_autoStop = true;

		if (m_use_caller && m_thread_count == 0
			&& (m_thread_substitude_caller_fiber->getState() == Fiber::TERMINAL || m_thread_substitude_caller_fiber->getState() == Fiber::INITIALIZE))
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << this << "stopped";
			//使用LoggerManager单例的默认logger输出日志
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::DEBUG, event);
		
			m_stopping = true;

			if (stopping())
			{
				return;
			}
		}

		////如果使用了调用者线程,当前调度器应为本调度器，否则报错
		//if (m_use_caller)
		//{
		//	if (GetThis() != this)
		//	{
		//		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		//		Assert(event);
		//	}
		//}
		////如果未使用调用者线程,当前调度器不应为本调度器，否则报错
		//else
		//{
		//	if (GetThis() == this)
		//	{
		//		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		//		Assert(event);
		//	}
		//}
		
		m_stopping = true;

		//对于线程池的每个线程都tickle()一下
		for (size_t i = 0; i < m_thread_count; ++i)
		{
			tickle();
		}
		//如果使用了调用者线程，调用者线程也应该tickle()一下
		if (m_use_caller)
		{
			tickle();
		}

		//如果使用了调用者线程
		if (m_use_caller)
		{
			if (!stopping())
			{
				//进行调用者的Scheduler::run()协程，使其行使与线程池内的线程相同的功能
				//m_caller_fiber->call();
				m_thread_substitude_caller_fiber->swapIn();
			}
		}

		//调用者的Scheduler::run()协程执行完毕以后，再让调用者线程等待线程池的其他线程
		vector<shared_ptr<Thread>> threads;		//线程池局部变量
		{
			//先监视互斥锁，保护m_threads
			ScopedLock<Mutex> lock(m_mutex);
			//将线程池复制到局部变量再运行，确保m_threads的自由
			threads.swap(m_threads);
		}
		//阻塞调用者线程，将线程池内的所有线程加入等待队列
		for (auto& thread : threads)
		{
			thread->join();
		}
	}


	void Scheduler::schedule(const shared_ptr<Fiber> fiber, const int thread_id)
	{
		//如果原本任务队列为空，则需要在添加任务后通知调度器有任务了
		bool need_tickle = m_tasks.empty();
		Task task(fiber, thread_id);
		{
			//先监视互斥锁，保护被schedule_nolock访问的成员
			ScopedLock<Mutex> lock(m_mutex);
			//如果原本任务队列为空，则需要在添加任务后通知调度器有任务了
			//need_tickle = schedule_nolock(fiber, thread_id);
			
			//如果该任务非空，加入任务队列
			if (task.m_fiber)
			{
				m_tasks.push_back(task);
			}
		}

		if (need_tickle)
		{
			//通知调度器有任务了
			tickle();
		}
	}

	//void Scheduler::schedule(const function<void()> callback, const int thread_id)
	//{
	//	//用回调函数作参数创建一个新的协程
	//	shared_ptr<Fiber> fiber(new Fiber(callback));

	//	//是否需要通知调度器有任务了
	//	bool need_tickle = false;
	//	{
	//		//先监视互斥锁，保护被schedule_nolock访问的成员
	//		ScopedLock<Mutex> lock(m_mutex);
	//		//如果原本任务队列为空，则需要在添加任务后通知调度器有任务了
	//		need_tickle = schedule_nolock(fiber, thread_id);
	//	}

	//	if (need_tickle)
	//	{
	//		//通知调度器有任务了
	//		tickle();
	//	}
	//}
	void Scheduler::schedule(const function<void()> callback, const int thread_id)
	{
		//用回调函数作参数创建一个新的协程
		shared_ptr<Fiber> fiber(new Fiber(callback));
		schedule(fiber, thread_id);
	}

	//bool Scheduler::schedule_nolock(const shared_ptr<Fiber> fiber, const int thread_id)
	//{
	//	//如果原本任务队列为空，则需要在添加任务后通知调度器有任务了
	//	bool need_tickle = m_tasks.empty();
	//	Task task(fiber, thread_id);

	//	//如果该任务非空，加入任务队列
	//	if (task.m_fiber)
	//	{
	//		m_tasks.push_back(task);
	//	}

	//	//返回是否需要通知调度器有任务了
	//	return need_tickle;
	//}

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

		//先将线程专属的当前调度器设置为本调度器（即使是调用者的m_caller_fiber也一样）
		//setThis();
		t_scheduler = this;

		//如果当前线程id不等于调用者线程id,说明不论是否使用了调用者线程，至少这个线程不是调用者线程
		if (GetThread_id() != m_caller_thread_id)
		{
			//为当前线程创建主协程，并将其设为线程的调度器的主协程
			t_scheduler_fiber = Fiber::GetThis().get();
		}
		//否则这个线程时调用者线程
		else
		{
			//将调度器的主协程设置为调用者线程的Scheduler::run()协程，使其完成和线程池内线程的主协程相同的功能
			t_scheduler_fiber = m_thread_substitude_caller_fiber.get();
		}

		//空闲协程
		shared_ptr<Fiber> idle_fiber(new Fiber(bind(&Scheduler::idle, this)));

		//任务容器
		Task task;

		//任务执行循环
		while (true)
		{
			//重置任务容器
			task.reset();
			//是否要通知其他线程来完成任务
			bool tickle_me = false;
			//是否取到了任务
			bool is_active = false;

			//从任务队列里面取出一个应该执行的任务
			{
				//先监视互斥锁，保护
				ScopedLock<Mutex> lock(m_mutex);

				//尝试取得任务
				auto iterator = m_tasks.begin();
				while(iterator != m_tasks.end())
				{
					//该任务已设置负责的线程的id，且与当前线程id不同，则不做这个任务，而是通知其他线程来完成
					//反之如果未设置线程id（-1），则所有线程都有义务做这个任务
					if(iterator->m_thread_in_charge_id!=-1 && iterator->m_thread_in_charge_id!=GetThread_id())
					{
						++iterator;
						//通知其他线程来完成
						tickle_me = true;
						continue;
					}

					//该任务应包含协程对象或回调函数，否则报错
					//if (!iterator->m_fiber && !iterator->m_callback)
					if (!iterator->m_fiber)
					{
						shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
						Assert(event);
					}

					//如果该任务包含的是协程对象，且该协程是执行状态，则不做这个任务
					//if (iterator->m_fiber && iterator->m_fiber->getState() == Fiber::EXECUTE)
					if (iterator->m_fiber->getState() == Fiber::EXECUTE)
					{
						++iterator;
						continue;
					}

					//否则该任务可以做，取出该任务
					task = *iterator;
					//将该任务从任务队列删除，并将迭代器移动一位
					m_tasks.erase(iterator++);
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
			if (task.m_fiber&&(task.m_fiber->getState() != Fiber::TERMINAL && task.m_fiber->getState() != Fiber::EXCEPT))
			{
				//切换到该协程执行
				task.m_fiber->swapIn();
				//活跃的线程数量减一
				--m_active_thread_count;

				//如果此时该协程为准备状态，再将其放入任务队列
				if (task.m_fiber->getState() == Fiber::READY)
				{
					schedule(task.m_fiber);
				}
				//否则如果该协程不是终止或异常状态，则将其设为挂起状态
				else if (task.m_fiber->getState() != Fiber::TERMINAL
					&& task.m_fiber->getState() != Fiber::EXCEPT)
				{
					task.m_fiber->setState(Fiber::HOLD);
				}

				//本次任务容器的功能已完成，将其重置
				task.reset();
			}
			//如果没有做任何任务
			else
			{
				//如果取到了任务，说明单纯是这个任务既不包含协程也不包含回调函数，是个空任务（摸会鱼）
				if (is_active)
				{
					//活跃的线程数量减一
					--m_active_thread_count;
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

						//如果这个线程是调用者线程，则在退出循环之前应将调度器主协程设为调用者线程的主协程，否则调用者线程的Scheduler::run()协程将无法正常返回
						if(GetThread_id() == m_caller_thread_id)
						{
							t_scheduler_fiber = Fiber::t_main_fiber.get();
						}

						break;
					}
					//否则执行空闲协程
					else
					{
						//空闲的线程数量加一
						++m_idle_thread_count;
						//切换到空闲协程执行
						idle_fiber->swapIn();
						//空闲的线程数量减一
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
		return m_autoStop && m_stopping && m_tasks.empty() && m_active_thread_count == 0;
	}

	//空闲协程的回调函数
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

	////设置当前调度器为本调度器（线程专属）
	//void Scheduler::setThis()
	//{
	//	t_scheduler = this;
	//}



	//当前调度器（线程专属）
	thread_local Scheduler* Scheduler::t_scheduler = nullptr;
	//当前调度器的主协程（线程专属）
	thread_local Fiber* Scheduler::t_scheduler_fiber = nullptr;
}
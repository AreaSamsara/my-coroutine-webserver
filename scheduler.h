#pragma once
#include <memory>

#include "fiber.h"
#include "thread.h"

namespace SchedulerSpace
{
	using namespace MutexSpace;
	using namespace ThreadSpace;
	using namespace FiberSpace;
	using std::vector;

	class Scheduler
	{
	public:
		//使用调用者线程时usecaller为true，否则为false
		Scheduler(size_t thread_count = 1, const bool use_caller = true, const string& name = "");
		virtual ~Scheduler();

		const string& getName()const { return m_name; }

		//启动调度器
		void start();
		//停止调度器
		void stop();

		template<class Fiber_or_Callback>
		void schedule(const Fiber_or_Callback fiber_or_callback,const int thread_id = -1)
		{
			//是否需要通知调度器有任务了
			bool need_tickle = false;
			{
				//先监视互斥锁，保护被schedule_nolock访问的成员
				ScopedLock<Mutex> lock(m_mutex);
				//如果原本任务队列为空，则需要在添加任务后通知调度器有任务了
				need_tickle = schedule_nolock(fiber_or_callback, thread_id);
			}

			if (need_tickle)
			{
				//通知调度器有任务了
				tickle();
			}
		}

		template<class InputIterator>
		void schedule(InputIterator begin, InputIterator end)
		{
			//是否需要通知调度器有任务了
			bool need_tickle = false;
			{
				//先监视互斥锁，保护被schedule_nolock访问的成员
				ScopedLock<Mutex> lock(m_mutex);
				while (begin != end)
				{
					//只要schedule_nolock有一次返回true，则需要通知调度器有任务了
					need_tickle = schedule_nolock(&*begin) || need_tickle;
					++begin;
				}
			}
			if (need_tickle)
			{
				//通知调度器有任务了
				tickle();
			}
		}
	public:
		//获取当前调度器（线程专属）
		static Scheduler* GetThis();
		//获取当前调度器的主协程（线程专属）
		static Fiber* GetMainFiber();
	protected:
		//通知调度器有任务了
		virtual void tickle();
		//调度器分配给线程池内线程的回调函数
		void run();
		//返回是否可以停止
		virtual bool stopping();
		//空闲协程的回调函数
		virtual void idle();
		//设置当前调度器为本调度器（线程专属）
		void setThis();
	private:
		template<class Fiber_or_Callback>
		bool schedule_nolock(const Fiber_or_Callback fiber_or_callback, const int thread_id = -1)
		{
			//如果原本任务队列为空，则需要在添加任务后通知调度器有任务了
			bool need_tickle = m_fibers.empty();
			Task task(fiber_or_callback, thread_id);

			//如果该任务非空，加入任务队列
			if (task.m_fiber || task.m_callback)
			{
				m_fibers.push_back(task);
			}

			//返回是否需要通知调度器有任务了
			return need_tickle;
		}
	private:
		class Task
		{
		public:
			//协程
			shared_ptr<Fiber> m_fiber;
			//回调函数
			function<void()> m_callback;
			//负责任务的线程的id（为-1时表示所有线程都有义务做这个任务）
			int m_thread_in_charge_id;
		public:
			Task(shared_ptr<Fiber> fiber,const int thread_id)
				:m_fiber(fiber),m_thread_in_charge_id(thread_id){}

			Task(shared_ptr<Fiber>* fiber,const int thread_id)
				:m_thread_in_charge_id(thread_id)
			{
				m_fiber.swap(*fiber);
			}

			Task(function<void()> callback, const int thread_id)
				:m_callback(callback), m_thread_in_charge_id(thread_id) {}

			Task(function<void()>* callback, const int thread_id)
				:m_thread_in_charge_id(thread_id)
			{
				m_callback.swap(*callback);
			}

			Task():m_thread_in_charge_id(-1){}

			//重置任务
			void reset()
			{
				m_fiber = nullptr;
				m_callback = nullptr;
				m_thread_in_charge_id = -1;
			}
		};
	private:
		//互斥锁
		Mutex m_mutex;
		//线程池
		vector<shared_ptr<Thread>> m_threads;
		//待执行的协程队列
		list<Task> m_fibers;	
		//调用者线程用于取代线程以执行Scheduler::run()的协程，仅使用调用者线程时有效
		shared_ptr<Fiber> m_caller_fiber;
		//是否使用调用者线程
		bool m_use_caller = true;
		//调度器名称
		string m_name;						
	protected:
		//调度器的线程id数组
		vector<int> m_thread_ids;
		//线程数量
		size_t m_thread_count=0;
		//正在工作的线程数量
		size_t m_active_thread_count=0;
		//空闲线程数量
		size_t m_idle_thread_count=0;
		//是否正处于停止状态
		bool m_stopping = true;
		//是否自动停止
		bool m_autoStop = false;
		//调用者线程的id，仅使用调用者线程时有效
		int m_caller_thread_id = 0;
	public:
		//当前调度器（线程专属）
		static thread_local Scheduler* t_scheduler;
		//当前调度器的主协程（线程专属）
		static thread_local Fiber* t_scheduler_fiber;
	};
}
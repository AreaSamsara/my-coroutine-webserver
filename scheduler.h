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

	/*
	* 协程调度器类调用方法：
	* 1.先调用构造函数创建Scheduler对象，选择是否使用调用者线程（若使用则调用者线程加入调度，其将在调用stop()方法后一起参与任务处理）
	* 2.调用start()方法启动任务调度器，此后线程池内的线程会自发地完成任务队列内的任务
	* 3.调用schedule()方法将需要完成的任务（协程或者回调函数）加入任务队列
	* 4.调用stop()方法停止调度器，在此过程中调用者线程才加入任务处理的工作。在等待任务队列的所有任务都完成后，调度器停止。
	*/

	//协程调度器类
	class Scheduler
	{
	private:
		//任务类
		class Task
		{
		public:
			//协程
			shared_ptr<Fiber> m_fiber;
			//负责任务的线程的id（为默认值-1时表示所有线程都有义务做这个任务）
			int m_thread_in_charge_id = -1;
		public:
			Task(shared_ptr<Fiber> fiber, const int thread_id)
				:m_fiber(fiber), m_thread_in_charge_id(thread_id) {}

			Task(shared_ptr<Fiber>* fiber, const int thread_id)
				:m_thread_in_charge_id(thread_id)
			{
				m_fiber.swap(*fiber);
			}

			Task(function<void()> callback, const int thread_id)
				:m_thread_in_charge_id(thread_id)
			{
				m_fiber.reset(new  Fiber(callback));
			}

			Task(function<void()>* callback, const int thread_id)
				:m_thread_in_charge_id(thread_id)
			{
				m_fiber.reset(new  Fiber(*callback));
			}

			Task() {}

			//重置任务
			void reset()
			{
				m_fiber = nullptr;
				m_thread_in_charge_id = -1;
			}
		};
	public:
		//thread_count需要在构造函数内部再确定，故不加const；使用调用者线程时usecaller为true，否则为false
		Scheduler(size_t thread_count, const bool use_caller = true, const string& name = "main_scheduler");
		virtual ~Scheduler();

		//获取调度器名称
		const string& getName()const { return m_name; }

		//启动调度器
		void start();
		//停止调度器
		void stop();

		//调度任务，将协程放入任务类，再将任务加入任务队列
		void schedule(const shared_ptr<Fiber> fiber, const int thread_id = -1);
		void schedule(const function<void()>& callback, const int thread_id = -1);
		template<class InputIterator>
		void schedule(InputIterator begin, InputIterator end, const int thread_id = -1)
		{
			while (begin != end)
			{
				//只要schedule_nolock有一次返回true，则需要通知调度器有任务了
				schedule(*begin,thread_id);
				++begin;
			}
		}
	protected:
		//调度器分配给线程池内线程的回调函数
		void run();
		//空闲协程的回调函数
		virtual void idle();
		//通知调度器有任务了
		virtual void tickle();
		//返回是否竣工
		virtual bool is_completed();

		//获取空闲线程数量
		virtual bool getIdle_thread_count()const { return m_idle_thread_count; }		
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
		//调用者线程的id，仅使用调用者线程时有效（无效时设为-1，默认无效）
		int m_caller_thread_id = -1;
	private:
		//互斥锁
		Mutex m_mutex;
		//线程池
		vector<shared_ptr<Thread>> m_threads;
		//待执行的协程队列
		list<Task> m_tasks;
		//调用者线程用于取代线程以执行Scheduler::run()的协程，仅使用调用者线程时有效
		shared_ptr<Fiber> m_thread_substitude_caller_fiber;
		//是否使用调用者线程
		bool m_use_caller = true;
		//调度器名称
		string m_name;
	public:
		//当前调度器（线程专属）
		static thread_local Scheduler* t_scheduler;
		//当前调度器的主协程（线程专属）
		static thread_local Fiber* t_scheduler_fiber;
	};
}
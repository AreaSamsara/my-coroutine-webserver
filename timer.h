#pragma once
#include <memory>
#include <set>
#include "thread.h"

namespace TimerSpace
{
	using namespace ThreadSpace;
	using std::enable_shared_from_this;
	using std::weak_ptr;
	using std::set;

	class TimerManager;

	//定时器
	class Timer :public enable_shared_from_this<Timer>
	{
		//friend class TimerManager;
	public:
		////取消定时器
		//bool cancel();
		////刷新定时器的绝对执行时间
		//bool refreshExecute_time();
		////重设定时器执行周期
		//bool resetRun_cycle(uint64_t ms, bool from_now);
	//private:
	public:
		/*Timer(const uint64_t run_cycle,const function<void()>& callback,const bool recurring,TimerManager* manager);
		Timer(const uint64_t run_cycle, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring, TimerManager* manager);*/
		Timer(const uint64_t run_cycle, const function<void()>& callback, const bool recurring);
		Timer(const uint64_t run_cycle, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring);
		Timer(const uint64_t execute_time);

		//获取私有成员
		bool isRecurring()const { return m_recurring; }
		uint64_t getRun_cycle()const { return m_run_cycle; }
		function<void()> getCallback()const { return m_callback; }
		uint64_t getExecute_time()const { return m_execute_time; }

		//修改私有成员
		void setRun_cycle(const uint64_t run_cycle) { m_run_cycle = run_cycle; }
		void setExecute_time(const uint64_t execute_time) { m_execute_time = execute_time; }
		void setCallback(const function<void()>& callback) { m_callback = callback; }
	private:
		//是否为循环定时器
		bool m_recurring = false;
		//执行周期
		uint64_t m_run_cycle = 0;
		//绝对执行时间，初始化为当前时间+执行周期
		uint64_t m_execute_time = 0;
		//回调函数
		function<void()> m_callback;
		//定时器管理者
		//TimerManager* m_manager = nullptr;
	private:
		/*class Comparator
		{
		public:
			bool operator()(const shared_ptr<Timer>& lhs, const shared_ptr<Timer>& rhs)const;
		};*/
	};




	class TimerManager
	{
		//friend class Timer;
	private:
		class Comparator
		{
		public:
			bool operator()(const shared_ptr<Timer>& lhs, const shared_ptr<Timer>& rhs)const;
		};
	public:
		TimerManager();
		virtual ~TimerManager() {}

		//添加定时器
		bool addTimer(shared_ptr<Timer> timer);
		
		//获取距离下一个定时器执行还需要的时间
		uint64_t getTimeUntilNextTimer();

		//获取到期的（需要执行的）定时器的回调函数列表
		void getExpired_callbacks(vector<function<void()>>& callbacks);
		//返回是否有定时器
		bool hasTimer();



		//取消定时器
		bool cancel(shared_ptr<Timer> timer);
		//刷新定时器的绝对执行时间
		bool refreshExecute_time(shared_ptr<Timer> timer);
		//重设定时器执行周期
		bool resetRun_cycle(shared_ptr<Timer> timer, const uint64_t run_cycle, const bool from_now);
	private:
		//检测服务器的时间是否被调后了
		bool detectClockRollover(uint64_t now_ms);
	private:
		//互斥锁（读/写锁）
		Mutex_Read_Write m_mutex;
		//定时器集合
		//set<shared_ptr<Timer>, Timer::Comparator> m_timers;
		set<shared_ptr<Timer>, Comparator> m_timers;
		//上次执行的时间
		uint64_t m_previous_time = 0;
	};
}
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
		friend class TimerManager;
	public:
		//取消定时器
		bool cancel();
		//刷新设置定时器的执行时间
		bool refresh();
		//重置定时器时间
		bool reset(uint64_t ms, bool from_now);
	private:
		Timer(const uint64_t ms,const function<void()>& callback,const bool recurring,TimerManager* manager);
		Timer(const uint64_t next);
	private:
		//是否为循环定时器
		bool m_recurring = false;
		//执行周期
		uint64_t m_ms = 0;
		//精确的执行时间，初始化为当前时间+执行周期
		uint64_t m_next = 0;
		//回调函数
		function<void()> m_callback;
		//定时器管理者
		TimerManager* m_manager = nullptr;
	private:
		class Comparator
		{
		public:
			bool operator()(const shared_ptr<Timer>& lhs, const shared_ptr<Timer>& rhs)const;
		};
	};




	class TimerManager
	{
		friend class Timer;
	public:
		TimerManager();
		virtual ~TimerManager();

		//添加定时器
		shared_ptr<Timer> addTimer(const uint64_t ms, const function<void()>& callback, const bool recurring);

		//添加条件定时器
		shared_ptr<Timer> addConditionTimer(const uint64_t ms, const function<void()>& callback,weak_ptr<void> weak_condition ,const bool recurring);
		//获取下一个定时器的执行时间
		uint64_t getNextTimer();

		//获取需要执行的定时器的回调函数列表
		void listExpireCallback(vector<function<void()>>& callbacks);
		//返回是否有定时器
		bool hasTimer();
	protected:
		//当有新的定时器插入到定时器集合的开头，执行此函数
		virtual void onTimerInsertedAtFront() = 0;
		//将定时器添加到管理器中
		void addTimer(shared_ptr<Timer> timer, WriteScopedLock<Mutex_Read_Write>& writelock);
		
	private:
		//检测服务器的时间是否被调后了
		bool detectClockRollover(uint64_t now_ms);
	private:
		//互斥锁（读/写锁）
		Mutex_Read_Write m_mutex;
		//定时器集合
		set<shared_ptr<Timer>, Timer::Comparator> m_timers;
		//是否触发了onTimerInsertedAtFront()
		bool m_tickled = false;
		//上次执行的时间
		uint64_t m_previous_time = 0;
	};
}
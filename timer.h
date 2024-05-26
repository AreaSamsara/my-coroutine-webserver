#pragma once
#include <memory>
#include <set>
#include "thread.h"

namespace TimerSpace
{
	using namespace ThreadSpace;
	using std::weak_ptr;
	using std::set;

	/*
	* 类关系：
	* Timer类内部只有读取或修改私有成员的方法，大部分复杂方法都位于TimerManager类内部
	* TimerManager类内部有装有多个定时器的定时器集合，以及多个操纵定时器的方法
	*/
	/*
	* 定时器系统调用方法：
	* 1.先创建独立的Timer对象并为其设置回调函数，
	* 2.再创建TimerManager对象用于管理Timer对象，
	* 3.调用addTimer方法可以将Timer对象加入TimerManager对象的定时器集合中
	*/


	//定时器
	class Timer
	{
	public:
		Timer(const uint64_t run_cycle, const function<void()>& callback, const bool recurring = false);
		Timer(const uint64_t run_cycle, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring = false);
		Timer(const uint64_t execute_time);

		//获取私有成员
		bool isRecurring()const { return m_is_recurring; }
		uint64_t getRun_cycle()const { return m_run_cycle; }
		function<void()> getCallback()const { return m_callback; }
		uint64_t getExecute_time()const { return m_execute_time; }

		//修改私有成员
		void setRun_cycle(const uint64_t run_cycle) { m_run_cycle = run_cycle; }
		void setExecute_time(const uint64_t execute_time) { m_execute_time = execute_time; }
		void setCallback(const function<void()>& callback) { m_callback = callback; }
	private:
		//静态私有方法：条件回调函数，用于辅助构造条件定时器（为先于构造函数调用，故设置为静态方法）
		static void condition_callback(weak_ptr<void> weak_condition, const function<void()>& callback);
	private:
		//是否为循环定时器
		bool m_is_recurring = false;
		//执行周期
		uint64_t m_run_cycle = 0;
		//绝对执行时间，初始化为当前时间+执行周期
		uint64_t m_execute_time = 0;
		//回调函数
		function<void()> m_callback;
	};



	//定时器管理者
	class TimerManager
	{
	private:
		//定时器排序类，按定时器的绝对执行时间从早到晚排序
		class Comparator
		{
		public:
			bool operator()(const shared_ptr<Timer>& left_timer, const shared_ptr<Timer>& right_timer)const;
		};
	public:
		TimerManager();
		virtual ~TimerManager() {}

		//添加定时器
		bool addTimer(const shared_ptr<Timer> timer);
		//取消定时器
		bool cancelTimer(const shared_ptr<Timer> timer);
		//刷新定时器的绝对执行时间
		bool refreshExecute_time(const shared_ptr<Timer> timer);
		//重设定时器执行周期
		bool resetRun_cycle(const shared_ptr<Timer> timer, const uint64_t run_cycle, const bool from_now);

		//返回是否有定时器
		bool hasTimer();
		//获取距离下一个定时器执行还需要的时间
		uint64_t getTimeUntilNextTimer();
		//获取到期的（需要执行的）定时器的回调函数列表
		void getExpired_callbacks(vector<function<void()>>& callbacks);
	private:
		//检测服务器的时间是否被调后了
		bool detectClockRollover(const uint64_t now_ms);
	private:
		//互斥锁（读/写锁）
		Mutex_Read_Write m_mutex;
		//定时器集合，按定时器的绝对执行时间从早到晚排序
		set<shared_ptr<Timer>, Comparator> m_timers;
		//上次执行的时间
		uint64_t m_previous_time = 0;
	};
}
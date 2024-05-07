#include "timer.h"

namespace TimerSpace
{
	using std::bind;

	static void OnTimer(weak_ptr<void> weak_condition, const function<void()>& callback)
	{
		shared_ptr<void> temp = weak_condition.lock();
		if (temp)
		{
			callback();
		}
	}

	Timer::Timer(const uint64_t run_cycle, const function<void()>& callback, const bool recurring, TimerManager* manager)
		:m_run_cycle(run_cycle), m_callback(callback), m_recurring(recurring), m_manager(manager)
	{
		//精确的执行时间，初始化为当前时间+执行周期
		m_execute_time = GetCurrentMS() + m_run_cycle;
	}

	//构造条件定时器
	Timer::Timer(const uint64_t run_cycle, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring, TimerManager* manager)
		:Timer(run_cycle, bind(&OnTimer, weak_condition, callback), recurring, manager){}

	Timer::Timer(const uint64_t execute_time) :m_execute_time(execute_time) {}

	//取消定时器
	bool Timer::cancel()
	{
		//先监视互斥锁，保护
		WriteScopedLock<Mutex_Read_Write> writelock(m_manager->m_mutex);
		if (m_callback)
		{
			m_callback = nullptr;
			//在定时器集合中查找当前定时器
			auto iterator = m_manager->m_timers.find(shared_from_this());
			//如果找到了则将其删除
			m_manager->m_timers.erase(iterator);
			return true;
		}
		return false;
	}

	//刷新定时器的精确执行时间
	bool Timer::refreshExecute_time()
	{
		//先监视互斥锁，保护
		WriteScopedLock<Mutex_Read_Write> writelock(m_manager->m_mutex);
		//如果回调函数为空，刷新失败，返回false
		if (!m_callback)
		{
			return false;
		}
		
		//在定时器集合中查找当前定时器
		auto iterator = m_manager->m_timers.find(shared_from_this());
		//如果未找到则刷新失败，返回false
		if (iterator == m_manager->m_timers.end())
		{
			return false;
		}
		//如果找到了则将其删除
		m_manager->m_timers.erase(iterator);

		//将当前计时器的精确执行时间刷新为当前时间+执行周期
		m_execute_time = GetCurrentMS() + m_run_cycle;
		//刷新完时间以后再重新添加到定时器集合中（保证有序性）
		m_manager->m_timers.insert(shared_from_this());

		//刷新成功，返回true
		return true;
	}

	//重设定时器执行周期
	bool Timer::resetRun_cycle(uint64_t ms, bool from_now)
	{
		//如果重设的值与原值相同且不是从当前时间开始计算
		if (ms == m_run_cycle && !from_now)
		{
			return true;
		}

		{
			//先监视互斥锁，保护
			WriteScopedLock<Mutex_Read_Write> writelock(m_manager->m_mutex);

			//如果回调函数为空，重置失败，返回false
			if (!m_callback)
			{
				return false;
			}

			//在定时器集合中查找当前定时器
			auto iterator = m_manager->m_timers.find(shared_from_this());
			//如果未找到则重置失败，返回false
			if (iterator == m_manager->m_timers.end())
			{
				return false;
			}

			//如果找到了则将其删除
			m_manager->m_timers.erase(iterator);

			uint64_t start = 0;
			if (from_now)
			{
				start = GetCurrentMS();
			}
			else
			{
				start = m_execute_time - m_run_cycle;
			}

			m_run_cycle = ms;
			m_execute_time = start + m_run_cycle;
		}

		//m_manager->addTimer(shared_from_this(), writelock);
		m_manager->addTimer(shared_from_this());
		return true;
	}

	bool Timer::Comparator::operator()(const shared_ptr<Timer>& lhs, const shared_ptr<Timer>& rhs)const
	{
		if (!lhs && !rhs)
		{
			return false;
		}
		if (!lhs)
		{
			return true;
		}
		if (!rhs)
		{
			return false;
		}
		if (lhs->m_execute_time < rhs->m_execute_time)
		{
			return true;
		}
		if (lhs->m_execute_time > rhs->m_execute_time)
		{
			return false;
		}
		//如果精确的执行时间相同，则直接按照默认的方法比较地址
		return lhs.get() < rhs.get();
	}






	TimerManager::TimerManager()
	{
		m_previous_time = GetCurrentMS();
	}

	//添加定时器
	//shared_ptr<Timer> TimerManager::addTimer(const uint64_t ms, const function<void()>& callback, const bool recurring)
	//{
	//	shared_ptr<Timer> timer(new Timer(ms, callback, recurring, this));

	//	//先监视互斥锁，保护
	//	//WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
	//	//addTimer(timer, writelock);
	//	addTimer(timer);

	//	return timer;
	//}


	//将定时器添加到管理器中
	//void TimerManager::addTimer(shared_ptr<Timer> timer, WriteScopedLock<Mutex_Read_Write>& writelock)
	//void TimerManager::addTimer(shared_ptr<Timer> timer)
	bool TimerManager::addTimer(shared_ptr<Timer> timer)
	{
		//先监视互斥锁，保护
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		//insert()返回值为一个pair，其first为一指向新插入元素的迭代器，其second表示是否成功
		auto iterator = m_timers.insert(timer).first;	//指向新插入timer的迭代器

		//由于set是有序容器，如果新插入的timer位于set的开头，说明其执行时间最靠前，是即将要执行的定时器
		//bool at_front = (iterator == m_timers.begin()) && !m_tickled;
		//bool at_front = (iterator == m_timers.begin());

		/*if (at_front)
		{
			m_tickled = true;
		}*/

		//writelock.unlock();

		/*if (at_front)
		{
			onTimerInsertedAtFront();
		}*/
		return iterator == m_timers.begin();
	}

	

	////添加条件定时器
	//shared_ptr<Timer> TimerManager::addConditionTimer(const uint64_t ms, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring)
	//{
	//	return addTimer(ms, bind(&OnTimer,weak_condition,callback),recurring);
	//}

	//获取距离下一个定时器执行还需要的时间
	uint64_t TimerManager::getTimeUntilNextTimer()
	{
		//先监视互斥锁，保护
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		//m_tickled = false;
		
		//如果不存在下一个定时器，返回unsigned long long的极大值
		if (m_timers.empty())
		{
			return ~0ull;
		}

		const shared_ptr<Timer>& next = *m_timers.begin();
		uint64_t now_ms = GetCurrentMS();

		//如果下一个定时器的执行时间在以前，说明该定时器延误了，立即执行之
		if (now_ms >= next->m_execute_time)
		{
			return 0;
		}
		//否则返回仍需等待的时间
		else
		{
			return next->m_execute_time - now_ms;
		}
	}

	//获取到期的（需要执行的）定时器的回调函数列表
	void TimerManager::getExpired_callbacks(vector<function<void()>>& expired_callbacks)
	{
		//获取当前时间
		uint64_t now_ms = GetCurrentMS();

		//如果不存在下一个定时器，直接返回
		{
			//先监视互斥锁，保护m_timers
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			if (m_timers.empty())
			{
				return;
			}
		}

		//先监视互斥锁，保护
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		//检查服务器的时间是否被调后了
		bool rollover = detectClockRollover(now_ms);
		//如果时间没有被调后且定时器集合的第一个定时器都尚不需要执行，则直接返回
		if (!rollover && ((*m_timers.begin())->m_execute_time > now_ms))
		{
			return;
		}

		shared_ptr<Timer> now_timer(new Timer(now_ms));

		//如果服务器时间被调后了，将expired_timers设为整个m_timers；否则将其中所有精确执行时间不大于now_ms的加入expired_timers
		auto iterator = rollover ? m_timers.end() : m_timers.upper_bound(now_timer);

		//已到期的定时器集合
		vector<shared_ptr<Timer>> expired_timers(m_timers.begin(), iterator);

		//删除已被expired_timers取走的定时器
		m_timers.erase(m_timers.begin(), iterator);

		//将callbacks的大小设置为与expired_timers一致
		expired_callbacks.reserve(expired_timers.size());
		
		//将expired_timers取到的所有定时器依次放入callbacks
		for (auto& expired_timer : expired_timers)
		{
			expired_callbacks.push_back(expired_timer->m_callback);
			//如果该定时器为循环定时器，在执行前应设置下一个周期的定时器
			if (expired_timer->m_recurring)
			{
				expired_timer->m_execute_time = now_ms + expired_timer->m_run_cycle;
				m_timers.insert(expired_timer);
			}
			//否则置空其回调函数
			else
			{
				expired_timer->m_callback = nullptr;
			}
		}
	}

	//返回是否有定时器
	bool TimerManager::hasTimer()
	{
		//先监视互斥锁，保护m_timers
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		return !m_timers.empty();
	}

	//检测服务器的时间是否被调后了
	bool TimerManager::detectClockRollover(uint64_t now_ms)
	{
		bool rollover = false;
		if (now_ms < m_previous_time && now_ms < (m_previous_time - 60 * 60 * 1000))
		{
			rollover = true;
		}
		m_previous_time = now_ms;
		return rollover;
	}
}
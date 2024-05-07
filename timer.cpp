#include "timer.h"

namespace TimerSpace
{
	using std::bind;


	//class Timer
	Timer::Timer(const uint64_t run_cycle, const function<void()>& callback, const bool recurring)
		:m_run_cycle(run_cycle), m_callback(callback), m_recurring(recurring)
	{
		//精确的执行时间，初始化为当前时间+执行周期
		m_execute_time = GetCurrentMS() + m_run_cycle;
	}

	//构造条件定时器
	Timer::Timer(const uint64_t run_cycle, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring)
		:Timer(run_cycle, bind(&OnTimer, weak_condition, callback), recurring) {}

	Timer::Timer(const uint64_t execute_time) :m_execute_time(execute_time) {}

	void Timer::OnTimer(weak_ptr<void> weak_condition, const function<void()>& callback)
	{
		shared_ptr<void> temp = weak_condition.lock();
		if (temp)
		{
			callback();
		}
	}




	//class TimerManager
	TimerManager::TimerManager()
	{
		m_previous_time = GetCurrentMS();
	}

	//定时器排序类，按定时器的绝对执行时间从早到晚排序
	bool TimerManager::Comparator::operator()(const shared_ptr<Timer>& left_timer, const shared_ptr<Timer>& right_timer)const
	{
		if (!left_timer && !right_timer)
		{
			return false;
		}
		if (!left_timer)
		{
			return true;
		}
		if (!right_timer)
		{
			return false;
		}
		if (left_timer->getExecute_time() < right_timer->getExecute_time())
		{
			return true;
		}
		if (left_timer->getExecute_time() > right_timer->getExecute_time())
		{
			return false;
		}
		//如果精确的执行时间相同，则直接按照默认的方法比较地址
		return left_timer.get() < right_timer.get();
	}

	//将定时器添加到管理器中
	bool TimerManager::addTimer(shared_ptr<Timer> timer)
	{
		//先监视互斥锁，保护m_timers
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		//insert()返回值为一个pair，其first为一指向新插入元素的迭代器，其second表示是否成功
		auto iterator = m_timers.insert(timer).first;	//指向新插入timer的迭代器

		//由于set是有序容器，如果新插入的timer位于set的开头，说明其执行时间最靠前，是即将要执行的定时器，此时返回true作为提醒
		return iterator == m_timers.begin();
	}

	//取消定时器
	bool TimerManager::cancel(shared_ptr<Timer> timer)
	{
		//先监视互斥锁，保护
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		auto callback = timer->getCallback();
		if (callback)
		{
			callback = nullptr;
			//在定时器集合中查找当前定时器
			auto iterator = m_timers.find(timer);
			//如果找到了则将其删除
			m_timers.erase(iterator);
			return true;
		}
		return false;
	}

	//刷新定时器的精确执行时间
	bool TimerManager::refreshExecute_time(shared_ptr<Timer> timer)
	{
		//先监视互斥锁，保护
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		auto callback = timer->getCallback();
		//如果回调函数为空，刷新失败，返回false
		if (!callback)
		{
			return false;
		}

		//在定时器集合中查找当前定时器
		auto iterator = m_timers.find(timer);
		//如果未找到则刷新失败，返回false
		if (iterator == m_timers.end())
		{
			return false;
		}
		//如果找到了则将其删除
		m_timers.erase(iterator);

		//将当前计时器的精确执行时间刷新为当前时间+执行周期
		timer->setExecute_time(GetCurrentMS() + timer->getRun_cycle());

		//刷新完时间以后再重新添加到定时器集合中（保证有序性）
		//m_timers.insert(timer);
		addTimer(timer);	//new

		//刷新成功，返回true
		return true;
	}

	//重设定时器执行周期
	bool TimerManager::resetRun_cycle(shared_ptr<Timer> timer, const uint64_t run_cycle, const bool from_now)
	{
		//如果重设的值与原值相同且不是从当前时间开始计算，则没有必要进行修改
		if (run_cycle == timer->getRun_cycle() && !from_now)
		{
			return true;
		}

		auto callback = timer->getCallback();
		{
			//先监视互斥锁，保护
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			//如果回调函数为空，重置失败，返回false
			if (!callback)
			{
				return false;
			}

			//在定时器集合中查找当前定时器
			auto iterator = m_timers.find(timer);
			//如果未找到则重置失败，返回false
			if (iterator == m_timers.end())
			{
				return false;
			}

			//如果找到了则将其删除
			m_timers.erase(iterator);

			uint64_t start = 0;
			if (from_now)
			{
				start = GetCurrentMS();
			}
			else
			{
				start = timer->getExecute_time() - timer->getRun_cycle();
			}

			timer->setRun_cycle(run_cycle);
			timer->setExecute_time(start + timer->getRun_cycle());
		}

		addTimer(timer);
		return true;
	}


	//返回是否有定时器
	bool TimerManager::hasTimer()
	{
		//先监视互斥锁，保护m_timers
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		return !m_timers.empty();
	}
	
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
		//if (now_ms >= next->m_execute_time)
		if (now_ms >= next->getExecute_time())	//new
		{
			return 0;
		}
		//否则返回仍需等待的时间
		else
		{
			//return next->m_execute_time - now_ms;
			return next->getExecute_time() - now_ms;	//new
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
		if (!rollover && ((*m_timers.begin())->getExecute_time() > now_ms))
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
			expired_callbacks.push_back(expired_timer->getCallback());
			//如果该定时器为循环定时器，在执行前应设置下一个周期的定时器
			if (expired_timer->isRecurring())
			{
				expired_timer->setExecute_time(now_ms + expired_timer->getRun_cycle());
				m_timers.insert(expired_timer);
			}
			//否则置空其回调函数
			else
			{
				expired_timer->getCallback() = nullptr;
			}
		}
	}


	//检测服务器的时间是否被调后了
	bool TimerManager::detectClockRollover(const uint64_t now_ms)
	{
		/*bool rollover = false;
		if (now_ms < m_previous_time && now_ms < (m_previous_time - 60 * 60 * 1000))
		{
			rollover = true;
		}*/

		//如果当前时间早于上一次执行的时间一个小时以上，则可以认为服务器时间被调后了
		bool rollover =  now_ms < (m_previous_time - 60 * 60 * 1000);
		//将上次执行时间设置为当前时间
		m_previous_time = now_ms;
		return rollover;
	}
}
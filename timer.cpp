#include "timer.h"

namespace TimerSpace
{
	using std::bind;


	//class Timer:public
	Timer::Timer(const uint64_t run_cycle, const function<void()>& callback, const bool recurring)
		:m_run_cycle(run_cycle), m_callback(callback), m_recurring(recurring)
	{
		//精确的执行时间，初始化为当前时间+执行周期
		m_execute_time = GetCurrentMS() + m_run_cycle;
	}
	//构造条件定时器
	Timer::Timer(const uint64_t run_cycle, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring)
		:Timer(run_cycle, bind(&condition_callback, weak_condition, callback), recurring) {}
	Timer::Timer(const uint64_t execute_time) :m_execute_time(execute_time) {}


	//class Timer:private static
	//条件回调函数，用于辅助构造条件定时器
	void Timer::condition_callback(weak_ptr<void> weak_condition, const function<void()>& callback)
	{
		//读取weak_ptr保存的内容
		shared_ptr<void> temp = weak_condition.lock();
		//如果内容有效，则照常执行回调函数
		if (temp)
		{
			callback();
		}
	}




	//class TimerManager:private class
	//定时器排序类，按定时器的绝对执行时间从早到晚排序
	bool TimerManager::Comparator::operator()(const shared_ptr<Timer>& left_timer, const shared_ptr<Timer>& right_timer)const
	{
		//如果右边的定时器为空，则返回false（因为此时即便左边的定时器也为空，仍返回false）
		if (!right_timer)
		{
			return false;
		}
		//否则说明右边的定时器不为空，此时若左边为空则返回true
		else if (!left_timer)
		{
			return true;
		}
		//否则说明二者皆非空，若二者的绝对执行时间不同，返回其比较结果
		else if (left_timer->getExecute_time() != right_timer->getExecute_time())
		{
			return left_timer->getExecute_time() < right_timer->getExecute_time();
		}
		//否则直接按照默认的方法比较地址
		else
		{
			return left_timer.get() < right_timer.get();
		}
	}


	//class TimerManager:public
	TimerManager::TimerManager()
	{
		//初始化上次执行时间为当前时间
		m_previous_time = GetCurrentMS();
	}

	//将定时器添加到管理器中
	bool TimerManager::addTimer(const shared_ptr<Timer> timer)
	{
		//先监视互斥锁，保护m_timers
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		//insert()返回值为一个pair，其first为一指向新插入元素的迭代器，其second表示是否成功
		auto iterator = m_timers.insert(timer).first;	//指向新插入timer的迭代器

		//由于set是有序容器，如果新插入的timer位于set的开头，说明其执行时间最靠前，是即将要执行的定时器，此时返回true作为提醒
		return iterator == m_timers.begin();
	}

	//取消定时器
	bool TimerManager::cancelTimer(const shared_ptr<Timer> timer)
	{
		////先监视互斥锁，保护
		//WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		auto callback = timer->getCallback();
		//如果定时器内部的回调函数不为空，取消之并返回true
		if (callback)
		{
			callback = nullptr;

			//先监视互斥锁，保护m_timers
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			//在定时器集合中查找当前定时器
			auto iterator = m_timers.find(timer);
			//如果找到了则将其删除
			m_timers.erase(iterator);
			return true;
		}
		//否则取消失败，返回false
		else
		{
			return false;
		}
	}

	//刷新定时器的绝对执行时间
	bool TimerManager::refreshExecute_time(const shared_ptr<Timer> timer)
	{
		////先监视互斥锁，保护
		//WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		auto callback = timer->getCallback();
		//如果定时器的回调函数为空，刷新失败，返回false
		if (!callback)
		{
			return false;
		}

		{
			//先监视互斥锁，保护m_timers
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			//在定时器集合中查找当前定时器
			auto iterator = m_timers.find(timer);
			//如果未找到则刷新失败，返回false
			if (iterator == m_timers.end())
			{
				return false;
			}
			//如果找到了则将其删除
			m_timers.erase(iterator);
		}

		//将当前计时器的精确执行时间刷新为当前时间+执行周期
		timer->setExecute_time(GetCurrentMS() + timer->getRun_cycle());

		//刷新完绝对执行时间以后再将定时器重新添加到定时器集合中（保证有序性）
		addTimer(timer);

		//刷新成功，返回true
		return true;
	}

	//重设定时器执行周期
	bool TimerManager::resetRun_cycle(const shared_ptr<Timer> timer, const uint64_t run_cycle, const bool from_now)
	{
		//如果重设的值与原值相同且不是从当前时间开始计算，则没有必要进行修改
		if (run_cycle == timer->getRun_cycle() && !from_now)
		{
			return true;
		}

		auto callback = timer->getCallback();
		//如果定时器的回调函数为空，重置失败，返回false
		if (!callback)
		{
			return false;
		}

		{
			//先监视互斥锁，保护m_timers
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			//在定时器集合中查找当前定时器
			auto iterator = m_timers.find(timer);
			//如果未找到则重置失败，返回false
			if (iterator == m_timers.end())
			{
				return false;
			}

			//如果找到了则将其删除
			m_timers.erase(iterator);	
		}

		//起始时间
		uint64_t start = 0;
		//如果从现在开始计算起始时间，则绝对执行时间设为当前时间+新的执行周期
		if (from_now)
		{
			start = GetCurrentMS();
		}
		//否则绝对执行时间按照原值调整等同于执行周期的改变量
		else
		{
			start = timer->getExecute_time() - timer->getRun_cycle();
		}
		//重设定时器的执行周期和绝对执行时间
		timer->setRun_cycle(run_cycle);
		timer->setExecute_time(start + timer->getRun_cycle());

		//重设完执行周期以后再将定时器重新添加到定时器集合中（保证有序性）
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
		//先监视互斥锁，保护m_timers
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
		
		//如果不存在下一个定时器，返回unsigned long long的极大值
		if (m_timers.empty())
		{
			return ~0ull;
		}

		//即将执行的下一个定时器
		const shared_ptr<Timer>& next = *m_timers.begin();
		//获取当前时间
		uint64_t now = GetCurrentMS();

		//如果下一个定时器的执行时间在以前，说明该定时器延误了，立即执行之
		if (now >= next->getExecute_time())
		{
			return 0;
		}
		//否则返回仍需等待的时间
		else
		{
			return next->getExecute_time() - now;
		}
	}

	//获取到期的（需要执行的）定时器的回调函数列表
	void TimerManager::getExpired_callbacks(vector<function<void()>>& expired_callbacks)
	{
		//获取当前时间
		uint64_t now = GetCurrentMS();

		//如果不存在下一个定时器，直接返回
		{
			//先监视互斥锁，保护m_timers
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			if (m_timers.empty())
			{
				return;
			}
		}

		//检查服务器的时间是否被调后了
		bool rollover = detectClockRollover(now);

		//如果时间没有被调后且定时器集合的第一个定时器都尚不需要执行，则直接返回
		{
			//先监视互斥锁，保护m_timers
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			if (!rollover && ((*m_timers.begin())->getExecute_time() > now))
			{
				return;
			}
		}

		//临时定时器，绝对执行时间被设为当前时间，用于辅助定时器集合的排序
		shared_ptr<Timer> now_timer(new Timer(now));

		//已到期的定时器集合
		vector<shared_ptr<Timer>> expired_timers;
		{
			//先监视互斥锁，保护
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			//如果服务器时间被调后了，将expired_timers设为整个m_timers；否则将其中所有精确执行时间不大于now的加入expired_timers
			auto iterator = rollover ? m_timers.end() : m_timers.upper_bound(now_timer);
			expired_timers = vector<shared_ptr<Timer>>(m_timers.begin(), iterator);

			//从定时器集合中删除已被expired_timers取走的定时器
			m_timers.erase(m_timers.begin(), iterator);
		}

		//将expired_callbacks的大小设置为与expired_timers一致
		expired_callbacks.reserve(expired_timers.size());

		//将expired_timers取到的所有定时器依次放入expired_callbacks
		for (auto& expired_timer : expired_timers)
		{
			expired_callbacks.push_back(expired_timer->getCallback());
			//如果该定时器为循环定时器，在执行前应设置下一个周期的定时器
			if (expired_timer->isRecurring())
			{
				expired_timer->setExecute_time(now + expired_timer->getRun_cycle());
				addTimer(expired_timer);
			}
			//否则置空其回调函数
			else
			{
				expired_timer->getCallback() = nullptr;
			}
		}
	}


	//class TimerManager:private
	//检测服务器的时间是否被调后了
	bool TimerManager::detectClockRollover(const uint64_t now)
	{
		//如果当前时间早于上一次执行的时间一个小时以上，则可以认为服务器时间被调后了
		bool rollover =  now < (m_previous_time - 60 * 60 * 1000);
		//将上次执行时间设置为当前时间
		m_previous_time = now;
		return rollover;
	}
}
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
		//��ȷ��ִ��ʱ�䣬��ʼ��Ϊ��ǰʱ��+ִ������
		m_execute_time = GetCurrentMS() + m_run_cycle;
	}

	//����������ʱ��
	Timer::Timer(const uint64_t run_cycle, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring, TimerManager* manager)
		:Timer(run_cycle, bind(&OnTimer, weak_condition, callback), recurring, manager){}

	Timer::Timer(const uint64_t execute_time) :m_execute_time(execute_time) {}

	//ȡ����ʱ��
	bool Timer::cancel()
	{
		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_manager->m_mutex);
		if (m_callback)
		{
			m_callback = nullptr;
			//�ڶ�ʱ�������в��ҵ�ǰ��ʱ��
			auto iterator = m_manager->m_timers.find(shared_from_this());
			//����ҵ�������ɾ��
			m_manager->m_timers.erase(iterator);
			return true;
		}
		return false;
	}

	//ˢ�¶�ʱ���ľ�ȷִ��ʱ��
	bool Timer::refreshExecute_time()
	{
		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_manager->m_mutex);
		//����ص�����Ϊ�գ�ˢ��ʧ�ܣ�����false
		if (!m_callback)
		{
			return false;
		}
		
		//�ڶ�ʱ�������в��ҵ�ǰ��ʱ��
		auto iterator = m_manager->m_timers.find(shared_from_this());
		//���δ�ҵ���ˢ��ʧ�ܣ�����false
		if (iterator == m_manager->m_timers.end())
		{
			return false;
		}
		//����ҵ�������ɾ��
		m_manager->m_timers.erase(iterator);

		//����ǰ��ʱ���ľ�ȷִ��ʱ��ˢ��Ϊ��ǰʱ��+ִ������
		m_execute_time = GetCurrentMS() + m_run_cycle;
		//ˢ����ʱ���Ժ���������ӵ���ʱ�������У���֤�����ԣ�
		m_manager->m_timers.insert(shared_from_this());

		//ˢ�³ɹ�������true
		return true;
	}

	//���趨ʱ��ִ������
	bool Timer::resetRun_cycle(uint64_t ms, bool from_now)
	{
		//��������ֵ��ԭֵ��ͬ�Ҳ��Ǵӵ�ǰʱ�俪ʼ����
		if (ms == m_run_cycle && !from_now)
		{
			return true;
		}

		{
			//�ȼ��ӻ�����������
			WriteScopedLock<Mutex_Read_Write> writelock(m_manager->m_mutex);

			//����ص�����Ϊ�գ�����ʧ�ܣ�����false
			if (!m_callback)
			{
				return false;
			}

			//�ڶ�ʱ�������в��ҵ�ǰ��ʱ��
			auto iterator = m_manager->m_timers.find(shared_from_this());
			//���δ�ҵ�������ʧ�ܣ�����false
			if (iterator == m_manager->m_timers.end())
			{
				return false;
			}

			//����ҵ�������ɾ��
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
		//�����ȷ��ִ��ʱ����ͬ����ֱ�Ӱ���Ĭ�ϵķ����Ƚϵ�ַ
		return lhs.get() < rhs.get();
	}






	TimerManager::TimerManager()
	{
		m_previous_time = GetCurrentMS();
	}

	//��Ӷ�ʱ��
	//shared_ptr<Timer> TimerManager::addTimer(const uint64_t ms, const function<void()>& callback, const bool recurring)
	//{
	//	shared_ptr<Timer> timer(new Timer(ms, callback, recurring, this));

	//	//�ȼ��ӻ�����������
	//	//WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
	//	//addTimer(timer, writelock);
	//	addTimer(timer);

	//	return timer;
	//}


	//����ʱ����ӵ���������
	//void TimerManager::addTimer(shared_ptr<Timer> timer, WriteScopedLock<Mutex_Read_Write>& writelock)
	//void TimerManager::addTimer(shared_ptr<Timer> timer)
	bool TimerManager::addTimer(shared_ptr<Timer> timer)
	{
		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		//insert()����ֵΪһ��pair����firstΪһָ���²���Ԫ�صĵ���������second��ʾ�Ƿ�ɹ�
		auto iterator = m_timers.insert(timer).first;	//ָ���²���timer�ĵ�����

		//����set����������������²����timerλ��set�Ŀ�ͷ��˵����ִ��ʱ���ǰ���Ǽ���Ҫִ�еĶ�ʱ��
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

	

	////���������ʱ��
	//shared_ptr<Timer> TimerManager::addConditionTimer(const uint64_t ms, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring)
	//{
	//	return addTimer(ms, bind(&OnTimer,weak_condition,callback),recurring);
	//}

	//��ȡ������һ����ʱ��ִ�л���Ҫ��ʱ��
	uint64_t TimerManager::getTimeUntilNextTimer()
	{
		//�ȼ��ӻ�����������
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		//m_tickled = false;
		
		//�����������һ����ʱ��������unsigned long long�ļ���ֵ
		if (m_timers.empty())
		{
			return ~0ull;
		}

		const shared_ptr<Timer>& next = *m_timers.begin();
		uint64_t now_ms = GetCurrentMS();

		//�����һ����ʱ����ִ��ʱ������ǰ��˵���ö�ʱ�������ˣ�����ִ��֮
		if (now_ms >= next->m_execute_time)
		{
			return 0;
		}
		//���򷵻�����ȴ���ʱ��
		else
		{
			return next->m_execute_time - now_ms;
		}
	}

	//��ȡ���ڵģ���Ҫִ�еģ���ʱ���Ļص������б�
	void TimerManager::getExpired_callbacks(vector<function<void()>>& expired_callbacks)
	{
		//��ȡ��ǰʱ��
		uint64_t now_ms = GetCurrentMS();

		//�����������һ����ʱ����ֱ�ӷ���
		{
			//�ȼ��ӻ�����������m_timers
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			if (m_timers.empty())
			{
				return;
			}
		}

		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		//����������ʱ���Ƿ񱻵�����
		bool rollover = detectClockRollover(now_ms);
		//���ʱ��û�б������Ҷ�ʱ�����ϵĵ�һ����ʱ�����в���Ҫִ�У���ֱ�ӷ���
		if (!rollover && ((*m_timers.begin())->m_execute_time > now_ms))
		{
			return;
		}

		shared_ptr<Timer> now_timer(new Timer(now_ms));

		//���������ʱ�䱻�����ˣ���expired_timers��Ϊ����m_timers�������������о�ȷִ��ʱ�䲻����now_ms�ļ���expired_timers
		auto iterator = rollover ? m_timers.end() : m_timers.upper_bound(now_timer);

		//�ѵ��ڵĶ�ʱ������
		vector<shared_ptr<Timer>> expired_timers(m_timers.begin(), iterator);

		//ɾ���ѱ�expired_timersȡ�ߵĶ�ʱ��
		m_timers.erase(m_timers.begin(), iterator);

		//��callbacks�Ĵ�С����Ϊ��expired_timersһ��
		expired_callbacks.reserve(expired_timers.size());
		
		//��expired_timersȡ�������ж�ʱ�����η���callbacks
		for (auto& expired_timer : expired_timers)
		{
			expired_callbacks.push_back(expired_timer->m_callback);
			//����ö�ʱ��Ϊѭ����ʱ������ִ��ǰӦ������һ�����ڵĶ�ʱ��
			if (expired_timer->m_recurring)
			{
				expired_timer->m_execute_time = now_ms + expired_timer->m_run_cycle;
				m_timers.insert(expired_timer);
			}
			//�����ÿ���ص�����
			else
			{
				expired_timer->m_callback = nullptr;
			}
		}
	}

	//�����Ƿ��ж�ʱ��
	bool TimerManager::hasTimer()
	{
		//�ȼ��ӻ�����������m_timers
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		return !m_timers.empty();
	}

	//����������ʱ���Ƿ񱻵�����
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
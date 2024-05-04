#include "timer.h"

namespace TimerSpace
{
	using std::bind;


	Timer::Timer(const uint64_t ms, const function<void()>& callback, const bool recurring, TimerManager* manager)
		:m_ms(ms), m_callback(callback), m_recurring(recurring), m_manager(manager)
	{
		m_next = GetCurrentMS() + m_ms;

	}

	Timer::Timer(const uint64_t next) :m_next(next) {}

	bool Timer::cancel()
	{
		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_manager->m_mutex);
		if (m_callback)
		{
			m_callback = nullptr;
			auto iterator = m_manager->m_timers.find(shared_from_this());
			m_manager->m_timers.erase(iterator);
			return true;
		}
		return false;
	}

	bool Timer::refresh()
	{
		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_manager->m_mutex);
		if (!m_callback)
		{
			return false;
		}
		
		auto iterator = m_manager->m_timers.find(shared_from_this());
		if (iterator == m_manager->m_timers.end())
		{
			return false;
		}

		m_manager->m_timers.erase(iterator);
		m_next = GetCurrentMS() + m_ms;
		m_manager->m_timers.insert(shared_from_this());
		return true;
	}

	bool Timer::reset(uint64_t ms, bool from_now)
	{
		if (ms == m_ms && !from_now)
		{
			return true;
		}

		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_manager->m_mutex);

		if (!m_callback)
		{
			return false;
		}
		

		auto iterator = m_manager->m_timers.find(shared_from_this());
		if (iterator == m_manager->m_timers.end())
		{
			return false;
		}

		m_manager->m_timers.erase(iterator);
		uint64_t start = 0;
		if (from_now)
		{
			start = GetCurrentMS();
		}
		else
		{
			start = m_next - m_ms;
		}

		m_ms = ms;
		m_next = start + m_ms;
		m_manager->addTimer(shared_from_this());
		//iterator = m_manager->m_timers.insert(shared_from_this()).first;

		//m_next = GetCurrentMS() + m_ms;
		//m_manager->m_timers.insert(shared_from_this());
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
		if (lhs->m_next < rhs->m_next)
		{
			return true;
		}
		if (lhs->m_next > rhs->m_next)
		{
			return true;
		}
		//�����ȷ��ִ��ʱ����ͬ����ֱ�Ӱ���Ĭ�ϵķ����Ƚϵ�ַ
		return lhs.get() < rhs.get();
	}


	TimerManager::TimerManager()
	{
		m_previous_time = GetCurrentMS();
	}
	TimerManager::~TimerManager()
	{

	}

	shared_ptr<Timer> TimerManager::addTimer(const uint64_t ms, const function<void()>& callback, const bool recurring)
	{
		shared_ptr<Timer> timer(new Timer(ms, callback, recurring, this));
		addTimer(timer);

		return timer;
	}

	static void OnTimer(weak_ptr<void> weak_condition,const function<void()>& callback)
	{
		shared_ptr<void> temp = weak_condition.lock();
		if (temp)
		{
			callback();
		}
	}

	shared_ptr<Timer> TimerManager::addConditionTimer(const uint64_t ms, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring)
	{
		return addTimer(ms, bind(&OnTimer,weak_condition,callback),recurring);
	}

	//��ȡ��һ����ʱ����ִ��ʱ��
	uint64_t TimerManager::getNextTimer()
	{
		//�ȼ��ӻ�����������
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		m_tickled = false;
		//�����������һ����ʱ��������unsigned long long�ļ���ֵ
		if (m_timers.empty())
		{
			return ~0ull;
		}

		const shared_ptr<Timer>& next = *m_timers.begin();
		uint64_t now_ms = GetCurrentMS();

		//�����һ����ʱ����ִ��ʱ������ǰ��˵���ö�ʱ�������ˣ�����ִ��֮
		if (now_ms >= next->m_next)
		{
			return 0;
		}
		//���򷵻�����ȴ���ʱ��
		else
		{
			return next->m_next - now_ms;
		}
	}

	void TimerManager::listExpireCallback(vector<function<void()>>& callbacks)
	{
		uint64_t now_ms = GetCurrentMS();
		vector<shared_ptr<Timer>> expired;

		{
			//�ȼ��ӻ�����������
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			//�����������һ����ʱ����ֱ�ӷ���
			if (m_timers.empty())
			{
				return;
			}
		}

		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		bool rollover = detectClockRollover(now_ms);
		if (!rollover && ((*m_timers.begin())->m_next > now_ms))
		{
			return;
		}

		shared_ptr<Timer> now_timer(new Timer(now_ms));

		auto iterator = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
		while (iterator != m_timers.end() && (*iterator)->m_next == now_ms)
		{
			++iterator;
		}
		expired.insert(expired.begin(), m_timers.begin(), iterator);
		m_timers.erase(m_timers.begin(), iterator);
		callbacks.reserve(expired.size());

		for (auto& timer : expired)
		{
			callbacks.push_back(timer->m_callback);
			if (timer->m_recurring)
			{
				timer->m_next = now_ms + timer->m_ms;
				m_timers.insert(timer);
			}
			else
			{
				timer->m_callback = nullptr;
			}
		}
	}

	bool TimerManager::hasTimer()
	{
		//�ȼ��ӻ�����������
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		return !m_timers.empty();
	}

	void TimerManager::addTimer(shared_ptr<Timer> timer)
	{
		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		//insert()����ֵΪһ��pair����firstΪһָ���²���Ԫ�صĵ���������second��ʾ�Ƿ�ɹ�
		auto iterator = m_timers.insert(timer).first;	//ָ���²���timer�ĵ�����

		//����set����������������²����timerλ��set�Ŀ�ͷ��˵����ִ��ʱ���ǰ���Ǽ���Ҫִ�еĶ�ʱ��
		bool at_front = (iterator == m_timers.begin()) && !m_tickled;
		if(at_front)
		{
			m_tickled = true;
		}

		writelock.unlock();

		if (at_front)
		{
			onTimerInsertedAtFront();
		}
	}

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
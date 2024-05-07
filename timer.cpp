#include "timer.h"

namespace TimerSpace
{
	using std::bind;


	//class Timer
	Timer::Timer(const uint64_t run_cycle, const function<void()>& callback, const bool recurring)
		:m_run_cycle(run_cycle), m_callback(callback), m_recurring(recurring)
	{
		//��ȷ��ִ��ʱ�䣬��ʼ��Ϊ��ǰʱ��+ִ������
		m_execute_time = GetCurrentMS() + m_run_cycle;
	}

	//����������ʱ��
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

	//��ʱ�������࣬����ʱ���ľ���ִ��ʱ����絽������
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
		//�����ȷ��ִ��ʱ����ͬ����ֱ�Ӱ���Ĭ�ϵķ����Ƚϵ�ַ
		return left_timer.get() < right_timer.get();
	}

	//����ʱ����ӵ���������
	bool TimerManager::addTimer(shared_ptr<Timer> timer)
	{
		//�ȼ��ӻ�����������m_timers
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		//insert()����ֵΪһ��pair����firstΪһָ���²���Ԫ�صĵ���������second��ʾ�Ƿ�ɹ�
		auto iterator = m_timers.insert(timer).first;	//ָ���²���timer�ĵ�����

		//����set����������������²����timerλ��set�Ŀ�ͷ��˵����ִ��ʱ���ǰ���Ǽ���Ҫִ�еĶ�ʱ������ʱ����true��Ϊ����
		return iterator == m_timers.begin();
	}

	//ȡ����ʱ��
	bool TimerManager::cancel(shared_ptr<Timer> timer)
	{
		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		auto callback = timer->getCallback();
		if (callback)
		{
			callback = nullptr;
			//�ڶ�ʱ�������в��ҵ�ǰ��ʱ��
			auto iterator = m_timers.find(timer);
			//����ҵ�������ɾ��
			m_timers.erase(iterator);
			return true;
		}
		return false;
	}

	//ˢ�¶�ʱ���ľ�ȷִ��ʱ��
	bool TimerManager::refreshExecute_time(shared_ptr<Timer> timer)
	{
		//�ȼ��ӻ�����������
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		auto callback = timer->getCallback();
		//����ص�����Ϊ�գ�ˢ��ʧ�ܣ�����false
		if (!callback)
		{
			return false;
		}

		//�ڶ�ʱ�������в��ҵ�ǰ��ʱ��
		auto iterator = m_timers.find(timer);
		//���δ�ҵ���ˢ��ʧ�ܣ�����false
		if (iterator == m_timers.end())
		{
			return false;
		}
		//����ҵ�������ɾ��
		m_timers.erase(iterator);

		//����ǰ��ʱ���ľ�ȷִ��ʱ��ˢ��Ϊ��ǰʱ��+ִ������
		timer->setExecute_time(GetCurrentMS() + timer->getRun_cycle());

		//ˢ����ʱ���Ժ���������ӵ���ʱ�������У���֤�����ԣ�
		//m_timers.insert(timer);
		addTimer(timer);	//new

		//ˢ�³ɹ�������true
		return true;
	}

	//���趨ʱ��ִ������
	bool TimerManager::resetRun_cycle(shared_ptr<Timer> timer, const uint64_t run_cycle, const bool from_now)
	{
		//��������ֵ��ԭֵ��ͬ�Ҳ��Ǵӵ�ǰʱ�俪ʼ���㣬��û�б�Ҫ�����޸�
		if (run_cycle == timer->getRun_cycle() && !from_now)
		{
			return true;
		}

		auto callback = timer->getCallback();
		{
			//�ȼ��ӻ�����������
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			//����ص�����Ϊ�գ�����ʧ�ܣ�����false
			if (!callback)
			{
				return false;
			}

			//�ڶ�ʱ�������в��ҵ�ǰ��ʱ��
			auto iterator = m_timers.find(timer);
			//���δ�ҵ�������ʧ�ܣ�����false
			if (iterator == m_timers.end())
			{
				return false;
			}

			//����ҵ�������ɾ��
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


	//�����Ƿ��ж�ʱ��
	bool TimerManager::hasTimer()
	{
		//�ȼ��ӻ�����������m_timers
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		return !m_timers.empty();
	}
	
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
		//if (now_ms >= next->m_execute_time)
		if (now_ms >= next->getExecute_time())	//new
		{
			return 0;
		}
		//���򷵻�����ȴ���ʱ��
		else
		{
			//return next->m_execute_time - now_ms;
			return next->getExecute_time() - now_ms;	//new
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
		if (!rollover && ((*m_timers.begin())->getExecute_time() > now_ms))
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
			expired_callbacks.push_back(expired_timer->getCallback());
			//����ö�ʱ��Ϊѭ����ʱ������ִ��ǰӦ������һ�����ڵĶ�ʱ��
			if (expired_timer->isRecurring())
			{
				expired_timer->setExecute_time(now_ms + expired_timer->getRun_cycle());
				m_timers.insert(expired_timer);
			}
			//�����ÿ���ص�����
			else
			{
				expired_timer->getCallback() = nullptr;
			}
		}
	}


	//����������ʱ���Ƿ񱻵�����
	bool TimerManager::detectClockRollover(const uint64_t now_ms)
	{
		/*bool rollover = false;
		if (now_ms < m_previous_time && now_ms < (m_previous_time - 60 * 60 * 1000))
		{
			rollover = true;
		}*/

		//�����ǰʱ��������һ��ִ�е�ʱ��һ��Сʱ���ϣ��������Ϊ������ʱ�䱻������
		bool rollover =  now_ms < (m_previous_time - 60 * 60 * 1000);
		//���ϴ�ִ��ʱ������Ϊ��ǰʱ��
		m_previous_time = now_ms;
		return rollover;
	}
}
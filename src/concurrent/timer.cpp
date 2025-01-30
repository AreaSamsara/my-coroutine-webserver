#include "concurrent/timer.h"

namespace TimerSpace
{
	using std::bind;

	// class Timer:public
	Timer::Timer(const uint64_t run_cycle, const function<void()> &callback, const bool recurring)
		: m_run_cycle(run_cycle), m_callback(callback), m_is_recurring(recurring)
	{
		// ��ȷ��ִ��ʱ�䣬��ʼ��Ϊ��ǰʱ��+ִ������
		m_execute_time = GetCurrentMS() + m_run_cycle;
	}
	// ����������ʱ��
	Timer::Timer(const uint64_t run_cycle, const function<void()> &callback, weak_ptr<void> weak_condition, const bool recurring)
		: Timer(run_cycle, bind(&condition_callback, weak_condition, callback), recurring) {}
	Timer::Timer(const uint64_t execute_time) : m_execute_time(execute_time) {}

	// class Timer:private static
	// �����ص����������ڸ�������������ʱ��
	void Timer::condition_callback(weak_ptr<void> weak_condition, const function<void()> &callback)
	{
		// ��ȡweak_ptr���������
		shared_ptr<void> temp = weak_condition.lock();
		// ���������Ч�����ճ�ִ�лص�����
		if (temp)
		{
			callback();
		}
	}

	// class TimerManager:private class
	// ��ʱ�������࣬����ʱ���ľ���ִ��ʱ����絽������
	bool TimerManager::Comparator::operator()(const shared_ptr<Timer> &left_timer, const shared_ptr<Timer> &right_timer) const
	{
		// ����ұߵĶ�ʱ��Ϊ�գ��򷵻�false����Ϊ��ʱ������ߵĶ�ʱ��ҲΪ�գ��Է���false��
		if (!right_timer)
		{
			return false;
		}
		// ����˵���ұߵĶ�ʱ����Ϊ�գ���ʱ�����Ϊ���򷵻�true
		else if (!left_timer)
		{
			return true;
		}
		// ����˵�����߽Էǿգ������ߵľ���ִ��ʱ�䲻ͬ��������ȽϽ��
		else if (left_timer->getExecute_time() != right_timer->getExecute_time())
		{
			return left_timer->getExecute_time() < right_timer->getExecute_time();
		}
		// ����ֱ�Ӱ���Ĭ�ϵķ����Ƚϵ�ַ
		else
		{
			return left_timer.get() < right_timer.get();
		}
	}

	// class TimerManager:public
	TimerManager::TimerManager()
	{
		// ��ʼ���ϴ�ִ��ʱ��Ϊ��ǰʱ��
		m_previous_time = GetCurrentMS();
	}

	// ����ʱ�����ӵ���������
	bool TimerManager::addTimer(const shared_ptr<Timer> timer)
	{
		// �ȼ��ӻ�����������m_timers
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

		// insert()����ֵΪһ��pair����firstΪһָ���²���Ԫ�صĵ���������second��ʾ�Ƿ�ɹ�
		auto iterator = m_timers.insert(timer).first; // ָ���²���timer�ĵ�����

		// ����set����������������²����timerλ��set�Ŀ�ͷ��˵����ִ��ʱ���ǰ���Ǽ���Ҫִ�еĶ�ʱ������ʱ����true��Ϊ����
		return iterator == m_timers.begin();
	}

	// ȡ����ʱ��
	bool TimerManager::cancelTimer(const shared_ptr<Timer> timer)
	{
		auto callback = timer->getCallback();
		// �����ʱ���ڲ��Ļص�������Ϊ�գ�ȡ��֮������true
		if (callback)
		{
			callback = nullptr;

			// �ȼ��ӻ�����������m_timers
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			// �ڶ�ʱ�������в��ҵ�ǰ��ʱ��
			auto iterator = m_timers.find(timer);
			// ����ҵ�������ɾ��
			m_timers.erase(iterator);
			return true;
		}
		// ����ȡ��ʧ�ܣ�����false
		else
		{
			return false;
		}
	}

	// ˢ�¶�ʱ���ľ���ִ��ʱ��
	bool TimerManager::refreshExecute_time(const shared_ptr<Timer> timer)
	{
		auto callback = timer->getCallback();
		// �����ʱ���Ļص�����Ϊ�գ�ˢ��ʧ�ܣ�����false
		if (!callback)
		{
			return false;
		}

		{
			// �ȼ��ӻ�����������m_timers
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			// �ڶ�ʱ�������в��ҵ�ǰ��ʱ��
			auto iterator = m_timers.find(timer);
			// ���δ�ҵ���ˢ��ʧ�ܣ�����false
			if (iterator == m_timers.end())
			{
				return false;
			}
			// ����ҵ�������ɾ��
			m_timers.erase(iterator);
		}

		// ����ǰ��ʱ���ľ�ȷִ��ʱ��ˢ��Ϊ��ǰʱ��+ִ������
		timer->setExecute_time(GetCurrentMS() + timer->getRun_cycle());

		// ˢ�������ִ��ʱ���Ժ��ٽ���ʱ���������ӵ���ʱ�������У���֤�����ԣ�
		addTimer(timer);

		// ˢ�³ɹ�������true
		return true;
	}

	// ���趨ʱ��ִ������
	bool TimerManager::resetRun_cycle(const shared_ptr<Timer> timer, const uint64_t run_cycle, const bool from_now)
	{
		// ��������ֵ��ԭֵ��ͬ�Ҳ��Ǵӵ�ǰʱ�俪ʼ���㣬��û�б�Ҫ�����޸�
		if (run_cycle == timer->getRun_cycle() && !from_now)
		{
			return true;
		}

		auto callback = timer->getCallback();
		// �����ʱ���Ļص�����Ϊ�գ�����ʧ�ܣ�����false
		if (!callback)
		{
			return false;
		}

		{
			// �ȼ��ӻ�����������m_timers
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			// �ڶ�ʱ�������в��ҵ�ǰ��ʱ��
			auto iterator = m_timers.find(timer);
			// ���δ�ҵ�������ʧ�ܣ�����false
			if (iterator == m_timers.end())
			{
				return false;
			}

			// ����ҵ�������ɾ��
			m_timers.erase(iterator);
		}

		// ��ʼʱ��
		uint64_t start = 0;
		// ��������ڿ�ʼ������ʼʱ�䣬�����ִ��ʱ����Ϊ��ǰʱ��+�µ�ִ������
		if (from_now)
		{
			start = GetCurrentMS();
		}
		// �������ִ��ʱ�䰴��ԭֵ������ͬ��ִ�����ڵĸı���
		else
		{
			start = timer->getExecute_time() - timer->getRun_cycle();
		}
		// ���趨ʱ����ִ�����ں;���ִ��ʱ��
		timer->setRun_cycle(run_cycle);
		timer->setExecute_time(start + timer->getRun_cycle());

		// ������ִ�������Ժ��ٽ���ʱ���������ӵ���ʱ�������У���֤�����ԣ�
		addTimer(timer);

		return true;
	}

	// �����Ƿ��ж�ʱ��
	bool TimerManager::hasTimer()
	{
		// �ȼ��ӻ�����������m_timers
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		return !m_timers.empty();
	}

	// ��ȡ������һ����ʱ��ִ�л���Ҫ��ʱ��
	uint64_t TimerManager::getTimeUntilNextTimer()
	{
		// �ȼ��ӻ�����������m_timers
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		// �����������һ����ʱ��������unsigned long long�ļ���ֵ
		if (m_timers.empty())
		{
			return ~0ull;
		}

		// ����ִ�е���һ����ʱ��
		const shared_ptr<Timer> &next = *m_timers.begin();
		// ��ȡ��ǰʱ��
		uint64_t now = GetCurrentMS();

		// �����һ����ʱ����ִ��ʱ������ǰ��˵���ö�ʱ�������ˣ�����ִ��֮
		if (now >= next->getExecute_time())
		{
			return 0;
		}
		// ���򷵻�����ȴ���ʱ��
		else
		{
			return next->getExecute_time() - now;
		}
	}

	// ��ȡ���ڵģ���Ҫִ�еģ���ʱ���Ļص������б�
	void TimerManager::getExpired_callbacks(vector<function<void()>> &expired_callbacks)
	{
		// ��ȡ��ǰʱ��
		uint64_t now = GetCurrentMS();

		// �����������һ����ʱ����ֱ�ӷ���
		{
			// �ȼ��ӻ�����������m_timers
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			if (m_timers.empty())
			{
				return;
			}
		}

		// ����������ʱ���Ƿ񱻵�����
		bool rollover = detectClockRollover(now);

		// ���ʱ��û�б������Ҷ�ʱ�����ϵĵ�һ����ʱ�����в���Ҫִ�У���ֱ�ӷ���
		{
			// �ȼ��ӻ�����������m_timers
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			if (!rollover && ((*m_timers.begin())->getExecute_time() > now))
			{
				return;
			}
		}

		// ��ʱ��ʱ��������ִ��ʱ�䱻��Ϊ��ǰʱ�䣬���ڸ�����ʱ�����ϵ�����
		shared_ptr<Timer> now_timer(new Timer(now));

		// �ѵ��ڵĶ�ʱ������
		vector<shared_ptr<Timer>> expired_timers;
		{
			// �ȼ��ӻ�����������
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			// ���������ʱ�䱻�����ˣ���expired_timers��Ϊ����m_timers�������������о�ȷִ��ʱ�䲻����now�ļ���expired_timers
			auto iterator = rollover ? m_timers.end() : m_timers.upper_bound(now_timer);
			expired_timers = vector<shared_ptr<Timer>>(m_timers.begin(), iterator);

			// �Ӷ�ʱ��������ɾ���ѱ�expired_timersȡ�ߵĶ�ʱ��
			m_timers.erase(m_timers.begin(), iterator);
		}

		// ��expired_callbacks�Ĵ�С����Ϊ��expired_timersһ��
		expired_callbacks.reserve(expired_timers.size());

		// ��expired_timersȡ�������ж�ʱ�����η���expired_callbacks
		for (auto &expired_timer : expired_timers)
		{
			expired_callbacks.push_back(expired_timer->getCallback());
			// ����ö�ʱ��Ϊѭ����ʱ������ִ��ǰӦ������һ�����ڵĶ�ʱ��
			if (expired_timer->isRecurring())
			{
				expired_timer->setExecute_time(now + expired_timer->getRun_cycle());
				addTimer(expired_timer);
			}
			// �����ÿ���ص�����
			else
			{
				expired_timer->getCallback() = nullptr;
			}
		}
	}

	// class TimerManager:private
	// ����������ʱ���Ƿ񱻵�����
	bool TimerManager::detectClockRollover(const uint64_t now)
	{
		// �����ǰʱ��������һ��ִ�е�ʱ��һ��Сʱ���ϣ��������Ϊ������ʱ�䱻������
		bool rollover = now < (m_previous_time - 60 * 60 * 1000);
		// ���ϴ�ִ��ʱ������Ϊ��ǰʱ��
		m_previous_time = now;
		return rollover;
	}
}
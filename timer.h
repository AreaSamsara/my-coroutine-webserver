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

	class Timer :public enable_shared_from_this<Timer>
	{
		friend class TimerManager;
	public:
		bool cancel();
		bool refresh();
		bool reset(uint64_t ms, bool from_now);
	private:
		Timer(const uint64_t ms,const function<void()>& callback,const bool recurring,TimerManager* manager);
		Timer(const uint64_t next);
	private:
		//�Ƿ�Ϊѭ����ʱ��
		bool m_recurring = false;
		//ִ������
		uint64_t m_ms = 0;
		//��ȷ��ִ��ʱ��
		uint64_t m_next = 0;
		function<void()> m_callback;
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

		shared_ptr<Timer> addTimer(const uint64_t ms, const function<void()>& callback, const bool recurring);

		shared_ptr<Timer> addConditionTimer(const uint64_t ms, const function<void()>& callback,weak_ptr<void> weak_condition ,const bool recurring);
		//��ȡ��һ����ʱ����ִ��ʱ��
		uint64_t getNextTimer();

		void listExpireCallback(vector<function<void()>>& callbacks);
		bool hasTimer();
	protected:
		virtual void onTimerInsertedAtFront() = 0;
		void addTimer(shared_ptr<Timer> timer);
		
	private:
		bool detectClockRollover(uint64_t now_ms);
	private:
		//����������/д����
		Mutex_Read_Write m_mutex;
		//��ʱ������
		set<shared_ptr<Timer>, Timer::Comparator> m_timers;
		bool m_tickled = false;
		uint64_t m_previous_time = 0;
	};
}
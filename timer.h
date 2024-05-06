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

	//��ʱ��
	class Timer :public enable_shared_from_this<Timer>
	{
		friend class TimerManager;
	public:
		//ȡ����ʱ��
		bool cancel();
		//ˢ�����ö�ʱ����ִ��ʱ��
		bool refresh();
		//���ö�ʱ��ʱ��
		bool reset(uint64_t ms, bool from_now);
	private:
		Timer(const uint64_t ms,const function<void()>& callback,const bool recurring,TimerManager* manager);
		Timer(const uint64_t next);
	private:
		//�Ƿ�Ϊѭ����ʱ��
		bool m_recurring = false;
		//ִ������
		uint64_t m_ms = 0;
		//��ȷ��ִ��ʱ�䣬��ʼ��Ϊ��ǰʱ��+ִ������
		uint64_t m_next = 0;
		//�ص�����
		function<void()> m_callback;
		//��ʱ��������
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

		//��Ӷ�ʱ��
		shared_ptr<Timer> addTimer(const uint64_t ms, const function<void()>& callback, const bool recurring);

		//���������ʱ��
		shared_ptr<Timer> addConditionTimer(const uint64_t ms, const function<void()>& callback,weak_ptr<void> weak_condition ,const bool recurring);
		//��ȡ��һ����ʱ����ִ��ʱ��
		uint64_t getNextTimer();

		//��ȡ��Ҫִ�еĶ�ʱ���Ļص������б�
		void listExpireCallback(vector<function<void()>>& callbacks);
		//�����Ƿ��ж�ʱ��
		bool hasTimer();
	protected:
		//�����µĶ�ʱ�����뵽��ʱ�����ϵĿ�ͷ��ִ�д˺���
		virtual void onTimerInsertedAtFront() = 0;
		//����ʱ����ӵ���������
		void addTimer(shared_ptr<Timer> timer, WriteScopedLock<Mutex_Read_Write>& writelock);
		
	private:
		//����������ʱ���Ƿ񱻵�����
		bool detectClockRollover(uint64_t now_ms);
	private:
		//����������/д����
		Mutex_Read_Write m_mutex;
		//��ʱ������
		set<shared_ptr<Timer>, Timer::Comparator> m_timers;
		//�Ƿ񴥷���onTimerInsertedAtFront()
		bool m_tickled = false;
		//�ϴ�ִ�е�ʱ��
		uint64_t m_previous_time = 0;
	};
}
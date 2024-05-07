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
		//friend class TimerManager;
	public:
		////ȡ����ʱ��
		//bool cancel();
		////ˢ�¶�ʱ���ľ���ִ��ʱ��
		//bool refreshExecute_time();
		////���趨ʱ��ִ������
		//bool resetRun_cycle(uint64_t ms, bool from_now);
	//private:
	public:
		/*Timer(const uint64_t run_cycle,const function<void()>& callback,const bool recurring,TimerManager* manager);
		Timer(const uint64_t run_cycle, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring, TimerManager* manager);*/
		Timer(const uint64_t run_cycle, const function<void()>& callback, const bool recurring);
		Timer(const uint64_t run_cycle, const function<void()>& callback, weak_ptr<void> weak_condition, const bool recurring);
		Timer(const uint64_t execute_time);

		//��ȡ˽�г�Ա
		bool isRecurring()const { return m_recurring; }
		uint64_t getRun_cycle()const { return m_run_cycle; }
		function<void()> getCallback()const { return m_callback; }
		uint64_t getExecute_time()const { return m_execute_time; }

		//�޸�˽�г�Ա
		void setRun_cycle(const uint64_t run_cycle) { m_run_cycle = run_cycle; }
		void setExecute_time(const uint64_t execute_time) { m_execute_time = execute_time; }
		void setCallback(const function<void()>& callback) { m_callback = callback; }
	private:
		//�Ƿ�Ϊѭ����ʱ��
		bool m_recurring = false;
		//ִ������
		uint64_t m_run_cycle = 0;
		//����ִ��ʱ�䣬��ʼ��Ϊ��ǰʱ��+ִ������
		uint64_t m_execute_time = 0;
		//�ص�����
		function<void()> m_callback;
		//��ʱ��������
		//TimerManager* m_manager = nullptr;
	private:
		/*class Comparator
		{
		public:
			bool operator()(const shared_ptr<Timer>& lhs, const shared_ptr<Timer>& rhs)const;
		};*/
	};




	class TimerManager
	{
		//friend class Timer;
	private:
		class Comparator
		{
		public:
			bool operator()(const shared_ptr<Timer>& lhs, const shared_ptr<Timer>& rhs)const;
		};
	public:
		TimerManager();
		virtual ~TimerManager() {}

		//��Ӷ�ʱ��
		bool addTimer(shared_ptr<Timer> timer);
		
		//��ȡ������һ����ʱ��ִ�л���Ҫ��ʱ��
		uint64_t getTimeUntilNextTimer();

		//��ȡ���ڵģ���Ҫִ�еģ���ʱ���Ļص������б�
		void getExpired_callbacks(vector<function<void()>>& callbacks);
		//�����Ƿ��ж�ʱ��
		bool hasTimer();



		//ȡ����ʱ��
		bool cancel(shared_ptr<Timer> timer);
		//ˢ�¶�ʱ���ľ���ִ��ʱ��
		bool refreshExecute_time(shared_ptr<Timer> timer);
		//���趨ʱ��ִ������
		bool resetRun_cycle(shared_ptr<Timer> timer, const uint64_t run_cycle, const bool from_now);
	private:
		//����������ʱ���Ƿ񱻵�����
		bool detectClockRollover(uint64_t now_ms);
	private:
		//����������/д����
		Mutex_Read_Write m_mutex;
		//��ʱ������
		//set<shared_ptr<Timer>, Timer::Comparator> m_timers;
		set<shared_ptr<Timer>, Comparator> m_timers;
		//�ϴ�ִ�е�ʱ��
		uint64_t m_previous_time = 0;
	};
}
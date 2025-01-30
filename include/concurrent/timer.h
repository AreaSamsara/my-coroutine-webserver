#pragma once
#include <memory>
#include <set>
#include "concurrent/thread.h"

namespace TimerSpace
{
	using namespace ThreadSpace;
	using std::set;
	using std::weak_ptr;

	/*
	 * ���ϵ��
	 * Timer���ڲ�ֻ�ж�ȡ���޸�˽�г�Ա�ķ������󲿷ָ��ӷ�����λ��TimerManager���ڲ�
	 * TimerManager���ڲ���װ�ж����ʱ���Ķ�ʱ�����ϣ��Լ�������ݶ�ʱ���ķ���
	 */
	/*
	 * ��ʱ��ϵͳ���÷�����
	 * 1.�ȴ���������Timer����Ϊ�����ûص�������
	 * 2.�ٴ���TimerManager�������ڹ���Timer����
	 * 3.����addTimer�������Խ�Timer�������TimerManager����Ķ�ʱ��������
	 */

	// ��ʱ��
	class Timer
	{
	public:
		Timer(const uint64_t run_cycle, const function<void()> &callback, const bool recurring = false);
		Timer(const uint64_t run_cycle, const function<void()> &callback, weak_ptr<void> weak_condition, const bool recurring = false);
		Timer(const uint64_t execute_time);

		// ��ȡ˽�г�Ա
		bool isRecurring() const { return m_is_recurring; }
		uint64_t getRun_cycle() const { return m_run_cycle; }
		function<void()> getCallback() const { return m_callback; }
		uint64_t getExecute_time() const { return m_execute_time; }

		// �޸�˽�г�Ա
		void setRun_cycle(const uint64_t run_cycle) { m_run_cycle = run_cycle; }
		void setExecute_time(const uint64_t execute_time) { m_execute_time = execute_time; }
		void setCallback(const function<void()> &callback) { m_callback = callback; }

	private:
		// ��̬˽�з����������ص����������ڸ�������������ʱ����Ϊ���ڹ��캯�����ã�������Ϊ��̬������
		static void condition_callback(weak_ptr<void> weak_condition, const function<void()> &callback);

	private:
		// �Ƿ�Ϊѭ����ʱ��
		bool m_is_recurring = false;
		// ִ������
		uint64_t m_run_cycle = 0;
		// ����ִ��ʱ�䣬��ʼ��Ϊ��ǰʱ��+ִ������
		uint64_t m_execute_time = 0;
		// �ص�����
		function<void()> m_callback;
	};

	// ��ʱ��������
	class TimerManager
	{
	private:
		// ��ʱ�������࣬����ʱ���ľ���ִ��ʱ����絽������
		class Comparator
		{
		public:
			bool operator()(const shared_ptr<Timer> &left_timer, const shared_ptr<Timer> &right_timer) const;
		};

	public:
		TimerManager();
		virtual ~TimerManager() {}

		// ���Ӷ�ʱ��
		bool addTimer(const shared_ptr<Timer> timer);
		// ȡ����ʱ��
		bool cancelTimer(const shared_ptr<Timer> timer);
		// ˢ�¶�ʱ���ľ���ִ��ʱ��
		bool refreshExecute_time(const shared_ptr<Timer> timer);
		// ���趨ʱ��ִ������
		bool resetRun_cycle(const shared_ptr<Timer> timer, const uint64_t run_cycle, const bool from_now);

		// �����Ƿ��ж�ʱ��
		bool hasTimer();
		// ��ȡ������һ����ʱ��ִ�л���Ҫ��ʱ��
		uint64_t getTimeUntilNextTimer();
		// ��ȡ���ڵģ���Ҫִ�еģ���ʱ���Ļص������б�
		void getExpired_callbacks(vector<function<void()>> &callbacks);

	private:
		// ����������ʱ���Ƿ񱻵�����
		bool detectClockRollover(const uint64_t now_ms);

	private:
		// ����������/д����
		Mutex_Read_Write m_mutex;
		// ��ʱ�����ϣ�����ʱ���ľ���ִ��ʱ����絽������
		set<shared_ptr<Timer>, Comparator> m_timers;
		// �ϴ�ִ�е�ʱ��
		uint64_t m_previous_time = 0;
	};
}
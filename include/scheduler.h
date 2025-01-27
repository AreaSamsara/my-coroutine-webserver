#pragma once
#include <memory>

#include "fiber.h"
#include "thread.h"

namespace SchedulerSpace
{
	using namespace MutexSpace;
	using namespace ThreadSpace;
	using namespace FiberSpace;
	using std::vector;

	/*
	* Э�̵���������÷�����
	* 1.�ȵ��ù��캯������Scheduler����ѡ���Ƿ�ʹ�õ������̣߳���ʹ����������̼߳�����ȣ��佫�ڵ���stop()������һ�������������
	* 2.����start()��������������������˺��̳߳��ڵ��̻߳��Է��������������ڵ�����
	* 3.����schedule()��������Ҫ��ɵ�����Э�̻��߻ص������������������
	* 4.����stop()����ֹͣ���������ڴ˹����е������̲߳ż����������Ĺ������ڵȴ�������е�����������ɺ󣬵�����ֹͣ��
	*/

	//Э�̵�������
	class Scheduler
	{
	//private:
	protected:
		//������
		class Task
		{
		public:
			Task(shared_ptr<Fiber> fiber, const int thread_id)
				:m_fiber(fiber), m_thread_in_charge_id(thread_id) {}

			Task(shared_ptr<Fiber>* fiber, const int thread_id)
				:m_thread_in_charge_id(thread_id)
			{
				m_fiber.swap(*fiber);
			}

			Task(function<void()> callback, const int thread_id)
				:m_thread_in_charge_id(thread_id)
			{
				m_fiber.reset(new  Fiber(callback));
			}

			Task(function<void()>* callback, const int thread_id)
				:m_thread_in_charge_id(thread_id)
			{
				m_fiber.reset(new  Fiber(*callback));
			}

			Task() {}

			//��������
			void reset()
			{
				m_fiber = nullptr;
				m_thread_in_charge_id = -1;
			}
		public:
			//Э��
			shared_ptr<Fiber> m_fiber;
			//����������̵߳�id��ΪĬ��ֵ-1ʱ��ʾ�����̶߳����������������
			int m_thread_in_charge_id = -1;
		};
	public:
		//thread_count��Ҫ�ڹ��캯���ڲ���ȷ�����ʲ���const��ʹ�õ������߳�ʱusecallerΪtrue������Ϊfalse
		Scheduler(size_t thread_count = 1, const bool use_caller = true, const string& name = "main_scheduler");
		virtual ~Scheduler();

		//��ȡ����������
		const string& getName()const { return m_name; }

		//����������
		void start();
		//ֹͣ������
		void stop();

		//�������񣬽�Э�̷��������࣬�ٽ���������������
		void schedule(const shared_ptr<Fiber> fiber, const int thread_id = -1);
		void schedule(const function<void()>& callback, const int thread_id = -1);
		template<class InputIterator>
		void schedule(InputIterator begin, InputIterator end, const int thread_id = -1)
		{
			while (begin != end)
			{
				//ֻҪschedule_nolock��һ�η���true������Ҫ֪ͨ��������������
				schedule(*begin,thread_id);
				++begin;
			}
		}
	public:
		//��̬��������ȡ��ǰ������
		static Scheduler* GetThis() { return t_scheduler; }
		//��̬�������޸ĵ�ǰ������
		static void SetThis(Scheduler* scheduler) { t_scheduler = scheduler; }
	protected:
		//������������̳߳����̵߳Ļص�����
		void run();
		//����Э�̵Ļص�����
		virtual void idle();
		//֪ͨ��������������
		virtual void tickle();
		//�����Ƿ񿢹�
		virtual bool isCompleted();

		//��ȡ�����߳�����
		virtual bool getIdle_thread_count()const { return m_idle_thread_count; }
	public:
		//��ǰ���������߳�ר����
		static thread_local Scheduler* t_scheduler;
		//��ǰ����������Э�̣��߳�ר����
		static thread_local Fiber* t_scheduler_fiber;
	protected:
		//���������߳�id����
		vector<int> m_thread_ids;
		//�߳�����
		size_t m_thread_count=0;
		//���ڹ������߳�����
		size_t m_active_thread_count=0;
		//�����߳�����
		size_t m_idle_thread_count=0;
		//�Ƿ�������ֹͣ״̬
		bool m_is_stopped = true;
		//�������̵߳�id����ʹ�õ������߳�ʱ��Ч����Чʱ��Ϊ-1��Ĭ����Ч��
		int m_caller_thread_id = -1;
	private:
		//������
		Mutex m_mutex;
		//�̳߳�
		vector<shared_ptr<Thread>> m_threads;
		//��ִ�е�Э�̶���
		list<Task> m_tasks;
		//�������߳�����ȡ���߳���ִ��Scheduler::run()��Э�̣���ʹ�õ������߳�ʱ��Ч
		shared_ptr<Fiber> m_thread_substitude_caller_fiber;
		//�Ƿ�ʹ�õ������߳�
		bool m_use_caller = true;
		//����������
		string m_name;
	};
}
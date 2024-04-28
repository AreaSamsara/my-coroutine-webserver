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

	class Scheduler
	{
	public:
		//thread_count��Ҫ�ڹ��캯���ڲ���ȷ�����ʲ���const��ʹ�õ������߳�ʱusecallerΪtrue������Ϊfalse
		Scheduler(size_t , const bool use_caller = true, const string& name = "");
		virtual ~Scheduler();

		const string& getName()const { return m_name; }

		//����������
		void start();
		//ֹͣ������
		void stop();

		//�������񣬽�Э�̷��������࣬�ٽ���������������
		void schedule(const shared_ptr<Fiber> fiber, const int thread_id = -1);
		void schedule(const function<void()> callback, const int thread_id = -1);
		template<class InputIterator>
		void schedule(InputIterator begin, InputIterator end)
		{
			//�Ƿ���Ҫ֪ͨ��������������
			bool need_tickle = false;
			{
				//�ȼ��ӻ�������������schedule_nolock���ʵĳ�Ա
				ScopedLock<Mutex> lock(m_mutex);
				while (begin != end)
				{
					//ֻҪschedule_nolock��һ�η���true������Ҫ֪ͨ��������������
					need_tickle = schedule_nolock(&*begin) || need_tickle;
					++begin;
				}
			}
			if (need_tickle)
			{
				//֪ͨ��������������
				tickle();
			}
		}
	protected:
		//֪ͨ��������������
		virtual void tickle();
		//������������̳߳����̵߳Ļص�����
		void run();
		//�����Ƿ񿢹�
		virtual bool is_completed();
		//����Э�̵Ļص�����
		virtual void idle();
	private:
		//������
		class Task
		{
		public:
			//Э��
			shared_ptr<Fiber> m_fiber;
			//����������̵߳�id��ΪĬ��ֵ-1ʱ��ʾ�����̶߳����������������
			int m_thread_in_charge_id = -1;
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

			Task(){}

			//��������
			void reset()
			{
				m_fiber = nullptr;
				m_thread_in_charge_id = -1;
			}
		};
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
		bool m_stopping = true;
		////�Ƿ��Զ�ֹͣ
		//bool m_autoStop = false;
		//�������̵߳�id����ʹ�õ������߳�ʱ��Ч����Чʱ��Ϊ-1��
		int m_caller_thread_id = -1;
	public:
		//��ǰ���������߳�ר����
		static thread_local Scheduler* t_scheduler;
		//��ǰ����������Э�̣��߳�ר����
		static thread_local Fiber* t_scheduler_fiber;
	};
}
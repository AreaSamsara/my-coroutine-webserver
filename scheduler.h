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
		//ʹ�õ������߳�ʱusecallerΪtrue������Ϊfalse
		Scheduler(size_t thread_count = 1, const bool use_caller = true, const string& name = "");
		virtual ~Scheduler();

		const string& getName()const { return m_name; }

		//����������
		void start();
		//ֹͣ������
		void stop();

		template<class Fiber_or_Callback>
		void schedule(const Fiber_or_Callback fiber_or_callback,const int thread_id = -1)
		{
			//�Ƿ���Ҫ֪ͨ��������������
			bool need_tickle = false;
			{
				//�ȼ��ӻ�������������schedule_nolock���ʵĳ�Ա
				ScopedLock<Mutex> lock(m_mutex);
				//���ԭ���������Ϊ�գ�����Ҫ����������֪ͨ��������������
				need_tickle = schedule_nolock(fiber_or_callback, thread_id);
			}

			if (need_tickle)
			{
				//֪ͨ��������������
				tickle();
			}
		}

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
	public:
		//��ȡ��ǰ���������߳�ר����
		static Scheduler* GetThis();
		//��ȡ��ǰ����������Э�̣��߳�ר����
		static Fiber* GetMainFiber();
	protected:
		//֪ͨ��������������
		virtual void tickle();
		//������������̳߳����̵߳Ļص�����
		void run();
		//�����Ƿ����ֹͣ
		virtual bool stopping();
		//����Э�̵Ļص�����
		virtual void idle();
		//���õ�ǰ������Ϊ�����������߳�ר����
		void setThis();
	private:
		template<class Fiber_or_Callback>
		bool schedule_nolock(const Fiber_or_Callback fiber_or_callback, const int thread_id = -1)
		{
			//���ԭ���������Ϊ�գ�����Ҫ����������֪ͨ��������������
			bool need_tickle = m_fibers.empty();
			Task task(fiber_or_callback, thread_id);

			//���������ǿգ������������
			if (task.m_fiber || task.m_callback)
			{
				m_fibers.push_back(task);
			}

			//�����Ƿ���Ҫ֪ͨ��������������
			return need_tickle;
		}
	private:
		class Task
		{
		public:
			//Э��
			shared_ptr<Fiber> m_fiber;
			//�ص�����
			function<void()> m_callback;
			//����������̵߳�id��Ϊ-1ʱ��ʾ�����̶߳����������������
			int m_thread_in_charge_id;
		public:
			Task(shared_ptr<Fiber> fiber,const int thread_id)
				:m_fiber(fiber),m_thread_in_charge_id(thread_id){}

			Task(shared_ptr<Fiber>* fiber,const int thread_id)
				:m_thread_in_charge_id(thread_id)
			{
				m_fiber.swap(*fiber);
			}

			Task(function<void()> callback, const int thread_id)
				:m_callback(callback), m_thread_in_charge_id(thread_id) {}

			Task(function<void()>* callback, const int thread_id)
				:m_thread_in_charge_id(thread_id)
			{
				m_callback.swap(*callback);
			}

			Task():m_thread_in_charge_id(-1){}

			//��������
			void reset()
			{
				m_fiber = nullptr;
				m_callback = nullptr;
				m_thread_in_charge_id = -1;
			}
		};
	private:
		//������
		Mutex m_mutex;
		//�̳߳�
		vector<shared_ptr<Thread>> m_threads;
		//��ִ�е�Э�̶���
		list<Task> m_fibers;	
		//�������߳�����ȡ���߳���ִ��Scheduler::run()��Э�̣���ʹ�õ������߳�ʱ��Ч
		shared_ptr<Fiber> m_caller_fiber;
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
		//�Ƿ��Զ�ֹͣ
		bool m_autoStop = false;
		//�������̵߳�id����ʹ�õ������߳�ʱ��Ч
		int m_caller_thread_id = 0;
	public:
		//��ǰ���������߳�ר����
		static thread_local Scheduler* t_scheduler;
		//��ǰ����������Э�̣��߳�ר����
		static thread_local Fiber* t_scheduler_fiber;
	};
}
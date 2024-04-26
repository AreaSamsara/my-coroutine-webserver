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
		virtual void idle();
		//���õ�ǰ������Ϊ�����������߳�ר����
		void setThis();
	private:
		template<class Fiber_or_Callback>
		bool schedule_nolock(const Fiber_or_Callback fiber_or_callback, const int thread_id = -1)
		{
			//���ԭ���������Ϊ�գ�����Ҫ����������֪ͨ��������������
			bool need_tickle = m_fibers.empty();
			Fiber_and_Thread fiber_and_thread(fiber_or_callback, thread_id);

			//���������ǿգ������������
			if (fiber_and_thread.m_fiber || fiber_and_thread.m_callback)
			{
				m_fibers.push_back(fiber_and_thread);
			}

			//�����Ƿ���Ҫ֪ͨ��������������
			return need_tickle;
		}
	private:
		class Fiber_and_Thread
		{
		public:
			//Э��
			shared_ptr<Fiber> m_fiber;
			//�ص�����
			function<void()> m_callback;
			//�߳�id
			int m_thread_id;
		public:
			Fiber_and_Thread(shared_ptr<Fiber> fiber,const int thread_id)
				:m_fiber(fiber),m_thread_id(thread_id){}

			Fiber_and_Thread(shared_ptr<Fiber>* fiber,const int thread_id)
				:m_thread_id(thread_id)
			{
				m_fiber.swap(*fiber);
			}

			Fiber_and_Thread(function<void()> callback, const int thread_id)
				:m_callback(callback), m_thread_id(thread_id) {}

			Fiber_and_Thread(function<void()>* callback, const int thread_id)
				:m_thread_id(thread_id)
			{
				m_callback.swap(*callback);
			}

			Fiber_and_Thread():m_thread_id(-1){}

			void reset()
			{
				m_fiber = nullptr;
				m_callback = nullptr;
				m_thread_id = -1;
			}
		};
	private:
		//������
		Mutex m_mutex;
		//�̳߳�
		vector<shared_ptr<Thread>> m_threads;
		//��ִ�е�Э�̶���
		list<Fiber_and_Thread> m_fibers;	
		//����������Э�̣���ʹ�õ������߳�ʱ��Ч
		shared_ptr<Fiber> m_main_fiber;
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
		//���̣߳��������̣߳���id����ʹ�õ������߳�ʱ��Ч
		int m_main_thread_id = 0;
	};
}
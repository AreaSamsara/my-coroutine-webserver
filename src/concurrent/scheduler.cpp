#include "concurrent/scheduler.h"
#include "common/log.h"
#include "concurrent/hook.h"

namespace SchedulerSpace
{
	using namespace HookSpace;
	using std::bind;

	// class Scheduler:public
	// thread_count��Ҫ�ڹ��캯���ڲ���ȷ�����ʲ���const��ʹ�õ������߳�ʱusecallerΪtrue������Ϊfalse
	Scheduler::Scheduler(size_t thread_count, const bool use_caller, const string &name)
		: m_use_caller(use_caller), m_name(name)
	{
		// �߳���Ӧ��Ϊ���������򱨴�
		if (thread_count <= 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		// ���ʹ�õ������߳�
		if (m_use_caller)
		{
			// �������߳�Ҳ�����߳��������̳߳ش�СӦ�ü�һ
			--thread_count;

			// Ϊ�������̴߳�����Э�̲�������Ϊ����������Э��
			t_scheduler_fiber = Fiber::GetThis().get();

			// ���õ������̵߳��߳������Э��(�������߳����Լ������񣬹�Ӧ������Э����ִ��Scheduler::run()��Ϊ���)
			m_thread_substitude_caller_fiber.reset(new Fiber(bind(&Scheduler::run, this)));

			// ���õ������̵߳�id������������Ϊ��Чֵ-1��
			m_caller_thread_id = GetThread_id();
			// ���������̵߳�id����������߳�id����
			m_thread_ids.push_back(m_caller_thread_id);
		}

		// �����߳��������ڹ��캯����ĩβ�Ա�֤׼ȷ�ԣ�
		m_thread_count = thread_count;
	}

	Scheduler::~Scheduler()
	{
		// ��ȻҪ�������������ǵ�����Ӧ�ô���ֹͣ״̬�����򱨴�
		if (!m_is_stopped)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}
		// ����̵߳ĵ�ǰ������Ϊ���������������ÿ�
		if (GetThis() == this)
		{
			SetThis(nullptr);
		}
	}

	// ����������
	void Scheduler::start()
	{
		// �ȼ��ӻ�����������m_stopping��m_threads��m_thread_count��m_thread_ids
		ScopedLock<Mutex> lock(m_mutex);

		// �������������ֹͣ״̬����Ӧ�ü���ִ��start����
		if (!m_is_stopped)
		{
			return;
		}
		// ��ֹͣ״̬�رգ�����ʼ���ȹ���
		m_is_stopped = false;

		// �̳߳��ڵ���ǰӦΪ�գ����򱨴�
		if (!m_threads.empty())
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		// ���̳߳ش�С��Ϊ�߳���
		m_threads.resize(m_thread_count);
		for (size_t i = 0; i < m_thread_count; ++i)
		{
			// ������Scheduler::runΪ�ص��������̲߳���������̳߳�
			m_threads[i].reset(new Thread(bind(&Scheduler::run, this), m_name + "_" + to_string(i)));
			// ��¼�����߳�id
			m_thread_ids.push_back(m_threads[i]->getId()); // �����̹߳��캯�������ź��������Դ˴�һ�����õ���ȷ���߳�id
		}
	}

	// ֹͣ������
	void Scheduler::stop()
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "stop";
		// ʹ��LoggerManager������Ĭ��logger�����־
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_DEBUG, log_event);

		// �����̳߳ص�ÿ���̶߳�tickle()һ��
		for (size_t i = 0; i < m_thread_count; ++i)
		{
			tickle();
		}
		// ���ʹ���˵������̣߳��������߳�ҲӦ��tickle()һ��
		if (m_use_caller)
		{
			tickle();
		}

		// �������ĵ��ȹ���ҵ�ѽ�����������Ϊֹͣ״̬��������ֻ��ȴ������߳�������񼴿�
		m_is_stopped = true;

		// ���ʹ���˵������߳���û�п������������Э�̼��빤��
		if (m_use_caller && !isCompleted())
		{
			// ���е����ߵ�Scheduler::run()Э�̣�ʹ����ʹ���̳߳��ڵ��߳���ͬ�Ĺ���
			m_thread_substitude_caller_fiber->swapIn();
		}

		// �����ߵ�Scheduler::run()Э��ִ������Ժ����õ������̵߳ȴ��̳߳ص������߳�ִ�����
		vector<shared_ptr<Thread>> threads; // �̳߳ؾֲ�����
		{
			// �ȼ��ӻ�����������m_threads
			ScopedLock<Mutex> lock(m_mutex);
			// ���̳߳ظ��Ƶ��ֲ����������У�ȷ��m_threads������
			threads.swap(m_threads);
		}
		// �����������̣߳����̳߳��ڵ������̼߳���ȴ�����
		for (auto &thread : threads)
		{
			thread->join();
		}
	}

	// �������񣬽�Э�̷��������࣬�ٽ���������������
	void Scheduler::schedule(const shared_ptr<Fiber> fiber, const int thread_id)
	{
		// ���ԭ���������Ϊ�գ�����Ҫ�����������֪ͨ��������������
		bool need_tickle = m_tasks.empty();

		Task task(fiber, thread_id);
		{
			// �ȼ��ӻ�����������m_tasks
			ScopedLock<Mutex> lock(m_mutex);

			// ���������ǿգ������������
			if (task.m_fiber)
			{
				m_tasks.push_back(task);
			}
		}

		if (need_tickle)
		{
			// ֪ͨ��������������
			tickle();
		}
	}
	void Scheduler::schedule(const function<void()> &callback, const int thread_id)
	{
		// �ûص���������������һ���µ�Э��
		shared_ptr<Fiber> fiber(new Fiber(callback));
		// �ø�Э����Ϊ����������һ���汾
		schedule(fiber, thread_id);
	}

	// class Scheduler:protected
	// ������������̳߳����̵߳Ļص�����
	void Scheduler::run()
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "run";
		// ʹ��LoggerManager������Ĭ��logger�����־
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_DEBUG, log_event);

		// set_hook_enable(true);
		t_hook_enable = true;

		// �Ƚ��߳�ר���ĵ�ǰ����������Ϊ������������ʹ�ǵ������̵߳��߳������Э��Ҳһ����
		SetThis(this);

		// �����ǰ�߳�id�����ڵ������߳�id,˵�������Ƿ�ʹ���˵������̣߳���������̲߳��ǵ������߳�
		if (GetThread_id() != m_caller_thread_id)
		{
			// Ϊ��ǰ�̴߳�����Э�̣���������Ϊ�̵߳ĵ���������Э��
			t_scheduler_fiber = Fiber::GetThis().get();
		}
		// ��������߳�Ϊ�������߳�
		else
		{
			// ������������Э������Ϊ�������̵߳�Scheduler::run()Э�̣�ʹ����ɺ��̳߳����̵߳���Э����ͬ�Ĺ���
			t_scheduler_fiber = m_thread_substitude_caller_fiber.get();
		}

		// �ÿ��лص�������������Э��
		shared_ptr<Fiber> idle_fiber(new Fiber(bind(&Scheduler::idle, this)));

		// ��������
		Task task;

		// ����ִ��ѭ��
		while (true)
		{
			// ������������
			task.reset();
			// �Ƿ�Ҫ֪ͨ�����߳����������
			bool tickle_me = false;
			// �Ƿ�ȡ��������
			bool is_active = false;

			// �������������ȡ��һ��Ӧ��ִ�е�����
			{
				// �ȼ��ӻ���������������Ա
				ScopedLock<Mutex> lock(m_mutex);

				// ����ȡ������
				auto iterator = m_tasks.begin();
				while (iterator != m_tasks.end())
				{
					// �����������ø�����̵߳�id�����뵱ǰ�߳�id��ͬ������������񣬶���֪ͨ�����߳������
					// Ȼ�����δ�����߳�id��-1�����������̶߳����������������
					if (iterator->m_thread_in_charge_id != -1 && iterator->m_thread_in_charge_id != GetThread_id())
					{
						++iterator;
						// ֪ͨ�����߳������
						tickle_me = true;
						continue;
					}

					// ����˵���������ɱ��̸߳����������Ӧ����Э�̶��󣬷��򱨴�
					if (!iterator->m_fiber)
					{
						shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
						Assert(log_event);
					}

					// ��������������Э�̶�����ִ��״̬�������������
					if (iterator->m_fiber->getState() == Fiber::EXECUTE)
					{
						++iterator;
						continue;
					}

					// ����������������ȡ��������
					task = *iterator;
					// ����������������ɾ���������������ƶ�һλ
					m_tasks.erase(iterator++);
					// ���ó������Ժ�Ӧ�����û�Ծ���߳�������һ
					++m_active_thread_count;

					// ��ʾ�Ѿ�ȡ���������˳���ȡ�����ѭ��
					is_active = true;
					break;
				}
			}

			// �������Ӧ���������߳���ɣ������tickle()
			if (tickle_me)
			{
				tickle();
			}

			// ���������Ҫ���Ҹ������Э�̲�����ֹ״̬���쳣״̬�����л�����Э��ִ��
			if (task.m_fiber && (task.m_fiber->getState() != Fiber::TERMINAL && task.m_fiber->getState() != Fiber::EXCEPT))
			{
				// �л�����Э��ִ��
				task.m_fiber->swapIn();
				// ִ����Ϻ��Ծ���߳�������һ
				--m_active_thread_count;

				// �����ʱ��Э��Ϊ׼��״̬�����ٴν�������������
				if (task.m_fiber->getState() == Fiber::READY)
				{
					schedule(task.m_fiber);
				}
				// ���������Э�̲�����ֹ���쳣״̬��������Ϊ����״̬
				else if (task.m_fiber->getState() != Fiber::TERMINAL && task.m_fiber->getState() != Fiber::EXCEPT)
				{
					task.m_fiber->setState(Fiber::HOLD);
				}

				// �������������Ĺ�������ɣ���������
				task.reset();
			}
			// ���û���κ�����Ҫ��
			else
			{
				// ���ȡ��������˵��������������񲻰���Э�̣��Ǹ��������Ǿ�������㣩
				if (is_active)
				{
					// ��Ծ���߳�������һ
					--m_active_thread_count;
				}
				// ����˵������������û�������ˣ�ִ�п���Э�̣����Ҽ�Ȼû��ȡ��������Ҳ����Ҫ���ٻ�Ծ�߳������ˣ�
				else
				{
					// �������Э��Ҳ�Ѿ�������ֹ״̬����������������ˣ��˳�ѭ��
					if (idle_fiber->getState() == Fiber::TERMINAL)
					{
						shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
						log_event->getSstream() << "idle_fiber terminated";
						// ʹ��LoggerManager������Ĭ��logger�����־
						Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_DEBUG, log_event);

						// �������߳��ǵ������̣߳������˳�ѭ��֮ǰӦ����������Э����Ϊ�������̵߳���Э�̣�����������̵߳�Scheduler::run()Э�̽��޷���������
						if (GetThread_id() == m_caller_thread_id)
						{
							t_scheduler_fiber = Fiber::t_main_fiber.get();
						}

						// �˳�ѭ��
						break;
					}
					// ����ִ�п���Э��
					else
					{
						// ִ��ǰ�����е��߳�������һ;
						{
							// �ȼ��ӻ�����������m_idle_thread_count
							ScopedLock<Mutex> lock(m_mutex);
							++m_idle_thread_count;
						}

						// �л�������Э��ִ��
						idle_fiber->swapIn();

						// ִ�к󽫿��е��߳�������һ
						{
							// �ȼ��ӻ�����������m_idle_thread_count
							ScopedLock<Mutex> lock(m_mutex);
							--m_idle_thread_count;
						}

						// �������Э�̲�����ֹ���쳣״̬��������Ϊ����״̬
						if (idle_fiber->getState() != Fiber::TERMINAL && idle_fiber->getState() != Fiber::EXCEPT)
						{
							idle_fiber->setState(Fiber::HOLD);
						}
					}
				}
			}
		}
	}

	// ����Э�̵Ļص�����
	void Scheduler::idle()
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "idle";
		// ʹ��LoggerManager������Ĭ��logger�����־
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_DEBUG, log_event);

		// �ڿ���֮ǰ����ֹ����Э�̶��ǽ������
		while (!isCompleted())
		{
			Fiber::YieldToHold();
		}
	}

	// ֪ͨ��������������
	void Scheduler::tickle()
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
		log_event->getSstream() << "tickle";
		// ʹ��LoggerManager������Ĭ��logger�����־
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
	}

	// �����Ƿ񿢹�
	bool Scheduler::isCompleted()
	{
		// �ȼ��ӻ��������������Ա
		ScopedLock<Mutex> lock(m_mutex);
		// ������ǰ���ǵ������ѹرա������б�Ϊ���һ�Ծ�߳���Ϊ0
		return m_is_stopped && m_tasks.empty() && m_active_thread_count == 0;
	}

	// class Scheduler:public static variable
	// ��ǰ���������߳�ר����
	thread_local Scheduler *Scheduler::t_scheduler = nullptr;
	// ��ǰ����������Э�̣��߳�ר����
	thread_local Fiber *Scheduler::t_scheduler_fiber = nullptr;
}
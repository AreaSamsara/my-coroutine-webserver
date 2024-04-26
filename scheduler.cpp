#include "scheduler.h"
#include "log.h"

namespace SchedulerSpace
{
	using std::bind;

	//��ǰ���������߳�ר����
	static thread_local Scheduler* t_scheduler = nullptr;
	//��ǰ����������Э�̣��߳�ר����
	static thread_local Fiber* t_scheduler_fiber = nullptr;

	//ʹ�õ������߳�ʱusecallerΪtrue������Ϊfalse
	Scheduler::Scheduler(size_t thread_count, const bool use_caller, const string& name)
		:m_name(name)
	{
		//�߳���Ӧ��Ϊ���������򱨴�
		if (thread_count <= 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//���ʹ�õ������߳�
		if (use_caller)
		{
			//��ȡ�������߳�
			Fiber::GetThis();
			//�������߳�Ҳ�����߳��������̳߳ش�СӦ�ü�һ
			--thread_count;


			if (GetThis() != nullptr)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}

			//���̵߳�ǰ����������Ϊ��������
			t_scheduler = this;

			//���õ���������Э��
			m_main_fiber.reset(new Fiber(bind(&Scheduler::run, this),true));
			//���̵߳�������Э������Ϊ����������Э��
			t_scheduler_fiber = m_main_fiber.get();

			//���õ��������߳�idΪ��ǰ�̣߳��������̣߳���id
			m_main_thread_id = GetThread_id();
			//�����������߳�id���������id����
			m_thread_ids.push_back(m_main_thread_id);
		}
		//�����ʹ�õ������߳�
		else
		{
			//�����������߳�id����Ϊ��Чֵ��-1��
			m_main_thread_id = -1;
		}

		//�����߳���
		m_thread_count = thread_count;
	}

	Scheduler::~Scheduler()
	{
		if (!m_stopping)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}
		if (GetThis() == this)
		{
			t_scheduler = nullptr;
		}
	}

	//����������
	void Scheduler::start()
	{
		//�ȼ��ӻ�����������
		ScopedLock<Mutex> lock(m_mutex);

		//�������������ֹͣ״̬����Ӧ�ü���ִ��start����
		if (!m_stopping)
		{
			return;
		}
		//��ֹͣ״̬�ر�
		m_stopping = false;

		//�̳߳�ӦΪ�գ����򱨴�
		if (!m_threads.empty())
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//���̳߳ش�С��Ϊ�߳���
		m_threads.resize(m_thread_count);
		for (size_t i = 0; i < m_thread_count; ++i)
		{
			//�����̲߳���������̳߳�
			m_threads[i].reset(new Thread(bind(&Scheduler::run, this), m_name + "_" + to_string(i)));
			//��¼�����߳�id
			m_thread_ids.push_back(m_threads[i]->getId());	//�����̹߳��캯�������ź��������Դ˴�һ�����õ���ȷ���߳�id
		}
	}

	//ֹͣ������
	void Scheduler::stop()
	{
		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "stop";
		//ʹ��LoggerManager������Ĭ��logger�����־
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

		m_autoStop = true;

		if (m_main_fiber && m_thread_count == 0
			&& (m_main_fiber->getState() == Fiber::TERMINAL || m_main_fiber->getState() == Fiber::INITIALIZE))
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			event->getSstream() << this << "stopped";
			//ʹ��LoggerManager������Ĭ��logger�����־
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
		
			m_stopping = true;

			if (stopping())
			{
				return;
			}
		}

		//���ʹ���˵������߳�
		if (m_main_thread_id != -1)
		{
			if (GetThis() != this)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}
		}
		//���δʹ�õ������߳�
		else
		{
			if (GetThis() == this)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}
		}
		
		m_stopping = true;

		for (size_t i = 0; i < m_thread_count; ++i)
		{
			tickle();
		}

		//���ʹ���˵������߳�
		if (m_main_fiber)
		{
			tickle();
		}

		//���ʹ���˵������߳�
		if (m_main_fiber)
		{
			if (!stopping())
			{
				m_main_fiber->call();
			}
		}

		vector<shared_ptr<Thread>> threads;

		{
			//�ȼ��ӻ�����������m_threads
			ScopedLock<Mutex> lock(m_mutex);
			//���̳߳ظ��Ƶ��ֲ����������У�ȷ��m_threads������
			threads.swap(m_threads);
		}

		//�����������е��̣߳����̳߳��ڵ������̼߳���ȴ�����
		for (auto& i : threads)
		{
			i->join();
		}
	}

	//֪ͨ��������������
	void Scheduler::tickle()
	{
		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "tickle";
		//ʹ��LoggerManager������Ĭ��logger�����־
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
	}

	//������������̳߳����̵߳Ļص�����
	void Scheduler::run()
	{
		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "run" ;
		//ʹ��LoggerManager������Ĭ��logger�����־
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::DEBUG, event);

		//�Ƚ��߳�ר���ĵ�ǰ����������Ϊ��������
		setThis();

		//�����ǰ�߳�id���������̣߳��������̣߳�id,˵�������Ƿ�ʹ���˵������̣߳���������̲߳��ǵ������߳�
		//���̵߳ĵ�������Э������Ϊ��ǰ�̵߳ĵ�ǰЭ��
		if (GetThread_id() != m_main_thread_id)
		{
			t_scheduler_fiber = Fiber::GetThis().get();
		}

		//����Э��
		shared_ptr<Fiber> idle_fiber(new Fiber(bind(&Scheduler::idle, this)));
		//�ɻص�����������Э��
		shared_ptr<Fiber> callback_fiber;

		//��������
		Fiber_and_Thread fiber_and_thread;

		//����ִ��ѭ��
		while (true)
		{
			//������������
			fiber_and_thread.reset();
			//�Ƿ�Ҫ֪ͨ�����߳����������
			bool tickle_me = false;
			//�Ƿ�ȡ��������
			bool is_active = false;

			//�������������ȡ��һ��Ӧ��ִ�е�����
			{
				//�ȼ��ӻ�����������
				ScopedLock<Mutex> lock(m_mutex);

				//����ȡ������
				auto iterator = m_fibers.begin();
				while(iterator != m_fibers.end())
				{
					//�������������߳�id�����뵱ǰ�߳�id��ͬ������������񣬶���֪ͨ�����߳������
					//��֮���δ�����߳�id��-1�����������̶߳����������������
					if(iterator->m_thread_id!=-1 && iterator->m_thread_id!=GetThread_id())
					{
						++iterator;
						//֪ͨ�����߳������
						tickle_me = true;
						continue;
					}

					//������Ӧ����Э�̶����ص����������򱨴�
					if (!iterator->m_fiber && !iterator->m_callback)
					{
						shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
						Assert(event);
					}

					//����������������Э�̶����Ҹ�Э����ִ��״̬�������������
					if (iterator->m_fiber && iterator->m_fiber->getState() == Fiber::EXECUTE)
					{
						++iterator;
						continue;
					}

					//����������������ȡ��������
					fiber_and_thread = *iterator;
					//����������������ɾ���������������ƶ�һλ
					m_fibers.erase(iterator++);
					//���ó������Ժ�Ӧ�����û�Ծ���߳�������һ
					++m_active_thread_count;

					//��ʾ�Ѿ�ȡ��������
					is_active = true;

					break;
				}
			}

			//�������Ӧ���������߳���ɣ������tickle()
			if (tickle_me)
			{
				tickle();
			}

			//���������Ҫ���������������Э���Ҹ�Э�̲�����ֹ״̬���쳣״̬�����л�����Э��ִ��
			if (fiber_and_thread.m_fiber && 
				(fiber_and_thread.m_fiber->getState() != Fiber::TERMINAL
				&& fiber_and_thread.m_fiber->getState() != Fiber::EXCEPT))
			{
				//�л�����Э��ִ��
				fiber_and_thread.m_fiber->swapIn();
				//��Ծ���߳�������һ
				--m_active_thread_count;

				//�����ʱ��Э��Ϊ׼��״̬���ٽ�������������
				if (fiber_and_thread.m_fiber->getState() == Fiber::READY)
				{
					schedule(fiber_and_thread.m_fiber);
				}
				//���������Э�̲�����ֹ���쳣״̬��������Ϊ����״̬
				else if (fiber_and_thread.m_fiber->getState() != Fiber::TERMINAL
					&& fiber_and_thread.m_fiber->getState() != Fiber::EXCEPT)
				{
					fiber_and_thread.m_fiber->setState(Fiber::HOLD);
				}

				//�������������Ĺ�������ɣ���������
				fiber_and_thread.reset();
			}
			//���������Ҫ��������������ǻص�������Ϊ�䴴��һ��Э�̣����л�����Э��ִ��
			else if (fiber_and_thread.m_callback)
			{
				//���callback_fiber��Ϊ�գ�����Fiber���reset��������֮
				if (callback_fiber)
				{
					callback_fiber->reset(fiber_and_thread.m_callback);
				}
				//���callback_fiberΪ�գ�����shared_ptr���reset��������֮
				else
				{
					callback_fiber.reset(new Fiber(fiber_and_thread.m_callback));
				}

				//�������������Ĺ�������ɣ���������
				fiber_and_thread.reset();
				
			
				//�л�����Э��ִ��
				callback_fiber->swapIn();
				//��Ծ���߳�������һ
				--m_active_thread_count;

				//�����ʱ��Э��Ϊ׼��״̬���ٽ�������������
				if (callback_fiber->getState() == Fiber::READY)
				{
					schedule(callback_fiber);
					callback_fiber.reset();
				}
				//���������Э������ֹ���쳣״̬���������
				else if (callback_fiber->getState() == Fiber::EXCEPT
					|| callback_fiber->getState() == Fiber::TERMINAL)
				{
					callback_fiber->reset(nullptr);
				}
				//�����Э�̲���׼������ֹ���쳣״̬��������Ϊ����״̬
				else
				{
					callback_fiber->setState(Fiber::HOLD);
					callback_fiber.reset();
				}
			}
			//���û�����κ�����
			else
			{
				//���ȡ��������˵���������������Ȳ�����Э��Ҳ�������ص��������Ǹ������������㣩
				if (is_active)
				{
					//��Ծ���߳�������һ
					--m_active_thread_count;
					//continue;
				}
				//����˵������������û�������ˣ�ִ�п���Э��
				else
				{
					//�������Э��Ҳ�Ѿ�������ֹ״̬����������������ˣ��˳�ѭ��
					if (idle_fiber->getState() == Fiber::TERMINAL)
					{
						shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
						event->getSstream() << "idle_fiber terminated";
						//ʹ��LoggerManager������Ĭ��logger�����־
						Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

						break;
					}
					//����ִ�п���Э��
					else
					{
						//��Ծ���߳�������һ
						++m_idle_thread_count;
						//�л�������Э��ִ��
						idle_fiber->swapIn();
						//��Ծ���߳�������һ
						--m_idle_thread_count;

						//���������Э�̲�����ֹ���쳣״̬��������Ϊ����״̬
						if (idle_fiber->getState() != Fiber::TERMINAL
							&& idle_fiber->getState() != Fiber::EXCEPT)
						{
							idle_fiber->setState(Fiber::HOLD);
						}
					}
				}
			}
		}
	}

	//�����Ƿ����ֹͣ
	bool Scheduler::stopping()
	{
		//�ȼ��ӻ�����������
		ScopedLock<Mutex> lock(m_mutex);
		return m_autoStop && m_stopping && m_fibers.empty() && m_active_thread_count == 0;
	}

	void Scheduler::idle()
	{
		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		event->getSstream() << "idle";
		//ʹ��LoggerManager������Ĭ��logger�����־
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

		//�ڿ���ֹ֮ͣǰ����ֹ����Э�̶��ǽ������
		while (!stopping())
		{
			Fiber::YieldTOHold();
		}
	}

	//���õ�ǰ������Ϊ�����������߳�ר����
	void Scheduler::setThis()
	{
		t_scheduler = this;
	}

	//��ȡ��ǰ���������߳�ר����
	Scheduler* Scheduler::GetThis()
	{
		return t_scheduler;
	}
	//��ȡ��ǰ����������Э�̣��߳�ר����
	Fiber* Scheduler::GetMainFiber()
	{
		return t_scheduler_fiber;
	}
}
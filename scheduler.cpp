#include "scheduler.h"
#include "log.h"

namespace SchedulerSpace
{
	using std::bind;

	//ʹ�õ������߳�ʱusecallerΪtrue������Ϊfalse
	Scheduler::Scheduler(size_t thread_count, const bool use_caller, const string& name)
		:m_use_caller(use_caller), m_name(name)
	{
		//�߳���Ӧ��Ϊ���������򱨴�
		if (thread_count <= 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//���ʹ�õ������߳�
		if (m_use_caller)
		{
			//��������ߵ���Э�̲����ڣ�����֮
			Fiber::GetThis();
			//�������߳�Ҳ�����߳��������̳߳ش�СӦ�ü�һ
			--thread_count;

			////�̵߳�ǰ������Ӧ��Ϊ�գ����򱨴�
			//if (GetThis() != nullptr)
			//{
			//	shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			//	Assert(event);
			//}

			//���̵߳�ǰ����������Ϊ��������
			//t_scheduler = this;

			//���õ������̵߳�Scheduler::runЭ��(�������߳����Լ������񣬹�Ӧ������Э����ִ��Scheduler::run()��Ϊ���)
			m_caller_fiber.reset(new Fiber(bind(&Scheduler::run, this),true));
			//���̵߳�������Э������Ϊ�������̵߳�m_caller_fiber(�����ǵ������̵߳���Э�̣�m_caller_fiberͬʱ��������������Э�̣�������Э�̵��л�)
			//t_scheduler_fiber = m_caller_fiber.get();
			t_scheduler_fiber = Fiber::t_main_fiber.get();	//new

			//���õ������̵߳�id
			m_caller_thread_id = GetThread_id();
			//���������̵߳�id���������id����
			m_thread_ids.push_back(m_caller_thread_id);
		}
		//�����ʹ�õ������߳�
		else
		{
			//���������̵߳�id����Ϊ��Чֵ��-1��
			m_caller_thread_id = -1;
		}

		//�����߳��������ڹ��캯����ĩβ�Ա�֤׼ȷ�ԣ�
		m_thread_count = thread_count;
	}

	Scheduler::~Scheduler()
	{
		//��ȻҪ�������������ǵ�����Ӧ�ô���ֹͣ״̬�����򱨴�
		if (!m_stopping)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}
		//����̵߳ĵ�ǰ������Ϊ���������������ÿ�
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
			//������Scheduler::runΪ�ص��������̲߳���������̳߳�
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
		Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::DEBUG, event);

		m_autoStop = true;

		if (m_use_caller && m_thread_count == 0
			&& (m_caller_fiber->getState() == Fiber::TERMINAL || m_caller_fiber->getState() == Fiber::INITIALIZE))
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

		////���ʹ���˵������߳�,��ǰ������ӦΪ�������������򱨴�
		//if (m_use_caller)
		//{
		//	if (GetThis() != this)
		//	{
		//		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		//		Assert(event);
		//	}
		//}
		////���δʹ�õ������߳�,��ǰ��������ӦΪ�������������򱨴�
		//else
		//{
		//	if (GetThis() == this)
		//	{
		//		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		//		Assert(event);
		//	}
		//}
		
		m_stopping = true;

		//�����̳߳ص�ÿ���̶߳�tickle()һ��
		for (size_t i = 0; i < m_thread_count; ++i)
		{
			tickle();
		}
		//���ʹ���˵������̣߳��������߳�ҲӦ��tickle()һ��
		if (m_use_caller)
		{
			tickle();
		}

		//���ʹ���˵������߳�
		if (m_use_caller)
		{
			if (!stopping())
			{
				//���Ƚ��е����ߵ�Scheduler::run()Э�̣�ʹ����ʹ���̳߳��ڵ��߳���ͬ�Ĺ���
				//m_caller_fiber->call();
				m_caller_fiber->swapIn();
			}
		}

		//�����ߵ�Scheduler::run()Э��ִ������Ժ����õ������̵߳ȴ��̳߳ص������߳�
		vector<shared_ptr<Thread>> threads;		//�̳߳ؾֲ�����
		{
			//�ȼ��ӻ�����������m_threads
			ScopedLock<Mutex> lock(m_mutex);
			//���̳߳ظ��Ƶ��ֲ����������У�ȷ��m_threads������
			threads.swap(m_threads);
		}
		//�����������̣߳����̳߳��ڵ������̼߳���ȴ�����
		for (auto& thread : threads)
		{
			thread->join();
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

		//�Ƚ��߳�ר���ĵ�ǰ����������Ϊ������������ʹ�ǵ����ߵ�m_caller_fiberҲһ����
		setThis();

		//�����ǰ�߳�id�����ڵ������߳�id,˵�������Ƿ�ʹ���˵������̣߳���������̲߳��ǵ������߳�
		if (GetThread_id() != m_caller_thread_id)
		{
			//Ϊ��ǰ�̴߳�����Э�̣���������Ϊ�̵߳ĵ���������Э��
			t_scheduler_fiber = Fiber::GetThis().get();
		}
		else
		{
			t_scheduler_fiber = m_caller_fiber.get();	//new
		}

		//����Э��
		shared_ptr<Fiber> idle_fiber(new Fiber(bind(&Scheduler::idle, this)));
		//�ɻص�����������Э��
		shared_ptr<Fiber> callback_fiber;

		//��������
		Task task;

		//����ִ��ѭ��
		while (true)
		{
			//������������
			task.reset();
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
					//�����������ø�����̵߳�id�����뵱ǰ�߳�id��ͬ������������񣬶���֪ͨ�����߳������
					//��֮���δ�����߳�id��-1�����������̶߳����������������
					if(iterator->m_thread_in_charge_id!=-1 && iterator->m_thread_in_charge_id!=GetThread_id())
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
					task = *iterator;
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
			if (task.m_fiber && 
				(task.m_fiber->getState() != Fiber::TERMINAL
				&& task.m_fiber->getState() != Fiber::EXCEPT))
			{
				//�л�����Э��ִ��
				task.m_fiber->swapIn();
				//��Ծ���߳�������һ
				--m_active_thread_count;

				//�����ʱ��Э��Ϊ׼��״̬���ٽ�������������
				if (task.m_fiber->getState() == Fiber::READY)
				{
					schedule(task.m_fiber);
				}
				//���������Э�̲�����ֹ���쳣״̬��������Ϊ����״̬
				else if (task.m_fiber->getState() != Fiber::TERMINAL
					&& task.m_fiber->getState() != Fiber::EXCEPT)
				{
					task.m_fiber->setState(Fiber::HOLD);
				}

				//�������������Ĺ�������ɣ���������
				task.reset();
			}
			//���������Ҫ��������������ǻص�������Ϊ�䴴��һ��Э�̣����л�����Э��ִ��
			else if (task.m_callback)
			{
				//���callback_fiber��Ϊ�գ�����Fiber���reset��������֮
				if (callback_fiber)
				{
					callback_fiber->reset(task.m_callback);
				}
				//���callback_fiberΪ�գ�����shared_ptr���reset��������֮
				else
				{
					callback_fiber.reset(new Fiber(task.m_callback));
				}

				//�������������Ĺ�������ɣ���������
				task.reset();
				
			
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


						if (m_use_caller)
						{
							t_scheduler_fiber = Fiber::t_main_fiber.get();		//new
						}

						break;
					}
					//����ִ�п���Э��
					else
					{
						//���е��߳�������һ
						++m_idle_thread_count;
						//�л�������Э��ִ��
						idle_fiber->swapIn();
						//���е��߳�������һ
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

	//����Э�̵Ļص�����
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

	//��ǰ���������߳�ר����
	thread_local Scheduler* Scheduler::t_scheduler = nullptr;
	//��ǰ����������Э�̣��߳�ר����
	thread_local Fiber* Scheduler::t_scheduler_fiber = nullptr;
}
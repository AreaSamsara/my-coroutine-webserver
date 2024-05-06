#include "iomanager.h"
#include <sys/epoll.h>
#include <fcntl.h>

namespace IOManagerSpace
{
	//��ȡ�¼���Ӧ���ﾳ
	IOManager::FileDescriptorContext::EventContext& IOManager::FileDescriptorContext::getContext(const Event event)
	{
		switch(event)
		{
		case READ:
			return m_read;
		case WRITE:
			return m_write;
		default:
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event,"getContext");
		}
	}

	//�����ﾳ
	void IOManager::FileDescriptorContext::resetContext(EventContext& event_context)
	{
		event_context.m_scheduler = nullptr;
		event_context.m_fiber.reset();
		event_context.m_callback = nullptr;
	}

	//�����¼�
	void IOManager::FileDescriptorContext::triggerEvent(Event event)
	{
		//Ҫ�������¼�ӦΪ��ע���¼������򱨴�
		if (!(m_events & event))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		//����ע���¼���ɾ��event
		m_events = (Event)(m_events & ~event);

		//�����õ���ʽ��ȡ�ļ��������ﾳ�е��¼��ﾳ
		EventContext& event_context = getContext(event);

		//����¼��ﾳ�лص�������������õ��ȷ���
		if (event_context.m_callback)
		{
			//event_context.m_scheduler->schedule(&event_context.m_callback);
			event_context.m_scheduler->schedule(event_context.m_callback);
		}
		//�������¼��ﾳ��Э�������õ��ȷ���
		else
		{
			//event_context.m_scheduler->schedule(&event_context.m_fiber);
			event_context.m_scheduler->schedule(event_context.m_fiber);
		}

		//�����¼��ﾳ�ĵ�����
		event_context.m_scheduler = nullptr;
	}

	IOManager::IOManager(size_t thread_count, const bool use_caller, const string& name)
		:Scheduler(thread_count,use_caller,name)
	{
		//����epoll�ļ�������
		m_epoll_file_descriptor = epoll_create(5000);	//�°汾��linuxϵͳ��epoll_create()��size�����Ѿ�û�������ˣ�ֻҪ��֤����0����
		//��epoll_create()�����ɹ���m_epoll_file_descriptorӦ������0�����򱨴�
		if (m_epoll_file_descriptor <= 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		
		//pipe()�����ܵ����ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴�
		if (pipe(m_pipe_file_descriptors) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		//���ù�עͨ�Źܵ��Ķ�ȡ�˵�epoll�¼���Ĭ����ӵ�epoll�ļ���������
		epoll_event epollevent;
		//���epollevent����
		memset(&epollevent, 0, sizeof(epoll_event));
		//��event����Ϊ�ɶ�����Ե����ģʽ��ֻ����״̬�����仯��ʱ��Żἤ��֪ͨ�����ظ�֪ͨ��
		epollevent.events = EPOLLIN | EPOLLET;
		//��עͨ�Źܵ��Ķ�ȡ��
		epollevent.data.fd = m_pipe_file_descriptors[0];

		//��ͨ�Źܵ��Ķ�ȡ������Ϊ������ģʽ���ɹ�����0��ʧ�ܷ���-1��ʧ���򱨴�
		if (fcntl(m_pipe_file_descriptors[0], F_SETFL, O_NONBLOCK) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}
		
		//��ͨ�Źܵ��Ķ�ȡ�˵�event�¼���ӵ�epoll�ļ��������У��ɹ�����0��ʧ�ܷ���-1��ʧ���򱨴�
		if (epoll_ctl(m_epoll_file_descriptor, EPOLL_CTL_ADD, m_pipe_file_descriptors[0], &epollevent) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		//�����ļ��������ﾳ������С
		resizeFile_descriptor_contexts(32);

		//IO�����߶��󴴽���Ϻ�Ĭ������
		start();
	}

	IOManager::~IOManager()
	{
		//IO�����߶���ʼ����ʱ��Ĭ��ֹͣ������
		stop();

		//�ر��ļ�������
		close(m_epoll_file_descriptor);
		close(m_pipe_file_descriptors[0]);
		close(m_pipe_file_descriptors[1]);

		//����ļ��������ﾳ����
		for (size_t i = 0; i < m_file_descriptor_contexts.size(); ++i)
		{
			if (m_file_descriptor_contexts[i])
			{
				delete m_file_descriptor_contexts[i];
			}
		}
	}

	//����¼����ɹ�����0��ʧ�ܷ���-1
	int IOManager::addEvent(const int file_descriptor, const Event event, function<void()> callback)
	{
		//�ļ��������ﾳ
		FileDescriptorContext* file_descriptor_context = nullptr;
		
		{
			////�ȼ��ӻ�����������
			//ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			////����ļ��������ﾳ������С�㹻�����ļ��������ﾳ����ȡ��
			//if (m_file_descriptor_contexts.size() > file_descriptor)
			//{
			//	file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
			//	//readlock.unlock();
			//}
			////����������������ȡ��
			//else
			//{
			//	//�Ƚ�����ȡ����������д����
			//	readlock.unlock();
			//	//�ȼ��ӻ�����������
			//	WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
			//	//ÿ�ν���С���䵽����ֵ��1.5��
			//	resizeFile_descriptor_contexts(file_descriptor * 1.5);
			//	file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
			//}
		}
		//����ļ��������ﾳ������С���㣬���Ƚ���������
		if (m_file_descriptor_contexts.size() <= file_descriptor)
		{
			//�ȼ��ӻ�����������
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
			//ÿ�ν���С���䵽����ֵ��1.5��
			resizeFile_descriptor_contexts(file_descriptor * 1.5);
		}
		{
			//�ȼ��ӻ�����������
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			//���ļ��������ﾳ��������ȡ��
			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//�ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);
		//Ҫ������¼���Ӧ���ѱ��ļ�������ע����¼������򱨴�
		if (file_descriptor_context->m_events & event)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			stringstream message_sstream;
			message_sstream << "addEvent error,file_descriptor=" << file_descriptor <<
				" event=" << event << " file_descriptor_context.event=" << file_descriptor_context->m_events;
			Assert(log_event,message_sstream.str());
		}

		//�����룺����ļ��������ﾳ�Ѿ�ע����¼���Ϊ�գ���ִ���޸��¼�������ִ������¼�
		int operation_code = file_descriptor_context->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

		//����epoll�¼�
		epoll_event epollevent;
		//��epollevent����Ϊ��Ե����ģʽ����������ע����¼�
		epollevent.events = EPOLLET | file_descriptor_context->m_events | event;
		epollevent.data.ptr = file_descriptor_context;

		//����epoll���ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴���addEvent()����-1
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return -1;
		}



		//��ǰ�ȴ�ִ�е��¼�������һ
		++m_pending_event_count;

		//���event����ע���¼��У�ע���ڿ���epoll֮��ִ�У���ȷ��epoll�����ѳɹ���
		file_descriptor_context->m_events = (Event)(file_descriptor_context->m_events | event);


		//�����õ���ʽ��ȡ�ļ��������ﾳ�е��¼��ﾳ
		FileDescriptorContext::EventContext& event_context = file_descriptor_context->getContext(event);

		//event_contextӦΪ�գ����򱨴�
		if (event_context.m_callback || event_context.m_fiber || event_context.m_scheduler)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		//�����¼��ﾳ�ĵ�����Ϊ��ǰ������
		event_context.m_scheduler = Scheduler::t_scheduler;
		//��������˻ص����������������¼��ﾳ�Ļص�����
		if (callback)
		{
			event_context.m_callback.swap(callback);
		}
		//�����¼��ﾳ��Э������Ϊ��ǰЭ��
		else
		{
			event_context.m_fiber = Fiber::GetThis();
			//�¼��ﾳ��Э��Ӧ������ִ��״̬�����򱨴�
			if (event_context.m_fiber->getState() != Fiber::EXECUTE)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}
		}
		
		//addEvent()��������ִ�У�����0
		return 0;
	}


	//ɾ���¼�
	bool IOManager::deleteEvent(const int file_descriptor, const Event event)
	{
		//�ļ��������ﾳ
		FileDescriptorContext* file_descriptor_context = nullptr;
		{
			//�ȼ��ӻ�����������
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			//���file_descriptor�������ļ��������ﾳ�����Ĵ�С����˵��û������ļ�������ƥ����¼�������false
			if ((int)m_file_descriptor_contexts.size() <= file_descriptor)
			{
				return false;
			}

			//�����ļ��������ﾳ����ȡ��
			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//�ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		//Ҫɾ�����¼�Ӧ���ѱ��ļ�������ע����¼������򷵻�false
		if (!(file_descriptor_context->m_events & event))
		{
			return false;
		}
		
		//Event new_events = (Event)(file_descriptor_context->m_events & ~event);

		//�����룺����ļ��������Ѿ�ע����¼���Ϊ�գ���ִ���޸��¼�������ִ��ɾ���¼�
		//int operation_code = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
		int operation_code = file_descriptor_context->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

		//����epoll�¼�
		epoll_event epollevent;
		//epollevent.events = EPOLLET | new_events;
		epollevent.events = EPOLLET | file_descriptor_context->m_events & ~event;
		epollevent.data.ptr = file_descriptor_context;

		//����epoll���ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴���deleteEvent()����false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}



		//��ǰ�ȴ�ִ�е��¼�������һ
		--m_pending_event_count;

		//����ע���¼���ɾ��event
		//file_descriptor_context->m_events = new_events;
		file_descriptor_context->m_events = (Event)(file_descriptor_context->m_events & ~event);

		//�����õ���ʽ��ȡ�ļ��������ﾳ�е��¼��ﾳ
		FileDescriptorContext::EventContext& event_context = file_descriptor_context->getContext(event);
		//���ø��ﾳ
		file_descriptor_context->resetContext(event_context);

		//deleteEvent()��������ִ�У�����true
		return true;
	}

	//ȡ���¼�
	bool IOManager::cancelEvent(const int file_descriptor, const Event event)
	{
		//�ļ��������ﾳ
		FileDescriptorContext* file_descriptor_context = nullptr;
		{
			//�ȼ��ӻ�����������
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			//���file_descriptor�������ļ��������ﾳ�����Ĵ�С����˵��û������ļ�������ƥ����¼�������false
			if ((int)m_file_descriptor_contexts.size() <= file_descriptor)
			{
				return false;
			}

			//�����ļ��������ﾳ����ȡ��
			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//�ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		//Ҫɾ�����¼�Ӧ���ѱ��ļ�������ע����¼������򷵻�false
		if (!(file_descriptor_context->m_events & event))
		{
			return false;
		}

		//Event new_events = (Event)(file_descriptor_context->m_events & ~event);

		//�����룺����ļ��������Ѿ�ע����¼���Ϊ�գ���ִ���޸��¼�������ִ��ɾ���¼�
		//int operation_code = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
		int operation_code = file_descriptor_context->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

		//����epoll�¼�
		epoll_event epollevent;
		//epollevent.events = EPOLLET | new_events;
		epollevent.events = EPOLLET | file_descriptor_context->m_events & ~event;
		epollevent.data.ptr = file_descriptor_context;

		//����epoll���ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴���deleteEvent()����false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}


		file_descriptor_context->triggerEvent(event);

		//��ǰ�ȴ�ִ�е��¼�������һ
		--m_pending_event_count;

		//cancelEvent()��������ִ�У�����true
		return true;
	}

	//ȡ�������¼�
	bool IOManager::cancelAllEvents(const int file_descriptor)
	{
		//�ļ��������ﾳ
		FileDescriptorContext* file_descriptor_context = nullptr;
		{
			//�ȼ��ӻ�����������
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			//���file_descriptor�������ļ��������ﾳ�����Ĵ�С����˵��û������ļ�������ƥ����¼�������false
			if ((int)m_file_descriptor_contexts.size() <= file_descriptor)
			{
				return false;
			}

			//�����ļ��������ﾳ����ȡ��
			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//�ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		//Ҫɾ��ȫ���¼����ļ��������ﾳ����ע���¼���ӦΪ�գ����򷵻�false
		if (!file_descriptor_context->m_events)
		{
			return false;
		}

		//�����룺ִ��ɾ���¼�
		int operation_code = EPOLL_CTL_DEL;

		//����epoll�¼�
		epoll_event epollevent;
		epollevent.events = 0;	//����Ϊ��
		epollevent.data.ptr = file_descriptor_context;

		//����epoll���ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴���cancelAllEvent()����false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		//������ע��READ�¼�������֮
		if (file_descriptor_context->m_events & READ)
		{
			file_descriptor_context->triggerEvent(READ);
			//��ǰ�ȴ�ִ�е��¼�������һ
			--m_pending_event_count;
		}
		//������ע��WRITE�¼�������֮
		if (file_descriptor_context->m_events & WRITE)
		{
			file_descriptor_context->triggerEvent(WRITE);
			//��ǰ�ȴ�ִ�е��¼�������һ
			--m_pending_event_count;
		}

		//������������ע���¼�ӦΪ�գ����򱨴�
		if (file_descriptor_context->m_events != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//cancelAllEvent()��������ִ�У�����true
		return true;
	}

	IOManager* IOManager::GetThis()
	{
		return dynamic_cast<IOManager*>(Scheduler::t_scheduler);
	}

	//֪ͨ��������������
	void IOManager::tickle()
	{
		//�����п����߳�ʱ�ŷ�����Ϣ
		if (getIdle_thread_count() > 0)
		{
			//��"T"д��ͨ�Źܵ���д���
			int return_value = write(m_pipe_file_descriptors[1], "T", 1);
			//���write()ʧ���򱨴�
			if (return_value != 1)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(log_event);
			}
		}
	}
	////�����Ƿ񿢹�
	//bool IOManager::is_completed()
	//{
	//	//������Scheduler::is_completed()��ǰ���£���ǰ�ȴ�ִ�е��¼�����ҲӦΪ0���ܿ���
	//	return m_pending_event_count == 0 && Scheduler::is_completed();
	//}
	//�����Ƿ񿢹�
	bool IOManager::is_completed()
	{
		//������Scheduler::is_completed()��ǰ���£���ǰ�ȴ�ִ�е��¼�����ҲӦΪ0���ܿ���
		uint64_t timeout = 0;
		return is_completed(timeout);
	}
	//�����Ƿ񿢹�
	bool IOManager::is_completed(uint64_t & timeout)
	{
		timeout = getNextTimer();
		return timeout == ~0ull && m_pending_event_count == 0 && Scheduler::is_completed();
	}


	//����Э�̵Ļص�����
	void IOManager::idle()
	{
		epoll_event* epollevents = new epoll_event[64]();
		shared_ptr<epoll_event> shared_events(epollevents, [](epoll_event* ptr) {delete[] ptr; });

		while (true)
		{
			uint64_t next_timeout = 0;
			//����Ѿ����������˳�ѭ��
			if (is_completed(next_timeout))
			{
				//next_timeout = getNextTimer();
				//if (next_timeout == ~0ull)
				{
					shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
					log_event->getSstream() << "name=" << getName() << " idle exit";
					Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
				}

				break;
			}

			int return_value = 0;
			//��������epoll_wait()�ȴ��¼���ֱ����epoll�¼����������߷������жϵĴ���
			do
			{
				//���ó�ʱʱ�䣨��̬������Ϊ3000ms
				static const int MAX_TIMEOUT = 3000;

				if (next_timeout != ~0ull)
				{
					next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
				}
				else
				{
					next_timeout = MAX_TIMEOUT;
				}

				//epoll_wait()���ؾ������ļ�����������,������������򷵻�-1
				//���ȴ�64��epoll�¼�����õȴ�next_timeout����ʱ����0
				return_value = epoll_wait(m_epoll_file_descriptor,epollevents, 64, next_timeout);

				//�����epoll�¼�������epoll_wait()�ȴ���ʱ�������жϵĴ������˳�ѭ��
				if (return_value >=0 || errno != EINTR)
				{
					break;
				}
			} while (true);

			vector<function<void()>> callbacks;
			//��ȡ��Ҫִ�еĶ�ʱ���Ļص������б�
			listExpireCallback(callbacks);
			//������Щ�ص�����
			if (!callbacks.empty())
			{
				schedule(callbacks.begin(), callbacks.end());
				/*for (auto& callback : callbacks)
				{
					schedule(callback);
				}*/
				callbacks.clear();
			}

			//���δ��������epoll�¼�
			for (size_t i = 0; i < return_value; ++i)
			{
				//�����õ���ʽ��ȡepoll�¼�
				epoll_event& epollevent = epollevents[i];

				//�����epoll�¼���ע��ͨ�Źܵ��Ķ�ȡ�ˣ���ֻ���ȡ����գ�������������ݣ���������ѭ��
				if (epollevent.data.fd == m_pipe_file_descriptors[0])
				{
					uint8_t dummy = 0;
					while (read(m_pipe_file_descriptors[0], &dummy, 1) == 1);

					continue;
				}

				//�ļ��������ﾳ
				FileDescriptorContext* file_descriptor_context =(FileDescriptorContext*)epollevent.data.ptr;
				//�ȼ��ӻ�����������
				ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

				//���epoll�¼���������������������Ѿ�׼���ã���epoll�¼�����Ϊ����������������
				if (epollevent.events & (EPOLLERR | EPOLLOUT))
				{
					epollevent.events |= EPOLLIN | EPOLLOUT;
				}

				//ʵ��Ҫִ�е��¼�
				int real_events = NONE;
				if (epollevent.events & EPOLLIN)
				{
					real_events |= READ;
				}
				if (epollevent.events & EPOLLOUT)
				{
					real_events |= WRITE;
				}

				//���ʵ��Ҫִ�е��¼���δ���ļ��������ﾳע�ᣬ��������ѭ��
				if ((file_descriptor_context->m_events & real_events) == NONE)
				{
					continue;
				}

				//�ļ��������ﾳ��ʵ��Ҫִ�е��¼����⣬ʣ�����ע���¼�
				int left_events = (file_descriptor_context->m_events & ~real_events);

				//�����룺���ʣ����ע���¼��¼���Ϊ�գ���ִ���޸��¼�������ִ��ɾ���¼�
				int operation_code = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
				//����epoll�¼�
				epollevent.events = EPOLLET | left_events;


				//����epoll���ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴���������ѭ��
				int rt2 = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor_context->m_file_descriptor, &epollevent);
				if (rt2)
				{
					shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
					log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code
						<< "," << file_descriptor_context->m_file_descriptor << "," << epollevent.events << "):"
						<< rt2 << " (" << errno << ") (" << strerror(errno) << ")";
					Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
					continue;
				}

				//����ʵ��Ҫִ�е�READ�¼�������֮
				if (real_events & READ)
				{
					file_descriptor_context->triggerEvent(READ);
					//��ǰ�ȴ�ִ�е��¼�������һ
					--m_pending_event_count;
				}
				//����ʵ��Ҫִ�е�WRITE�¼�������֮
				if (real_events & WRITE)
				{
					file_descriptor_context->triggerEvent(WRITE);
					//��ǰ�ȴ�ִ�е��¼�������һ
					--m_pending_event_count;
				}
			}

			//��ȡ��ǰЭ��
			shared_ptr<Fiber> current = Fiber::GetThis();
			//���л�����̨֮ǰ������ָ���л�Ϊ��ָ�룬��ֹshared_ptr�ļ�������������������������
			auto raw_ptr = current.get();
			//���shared_ptr
			current.reset();
			//ʹ����ָ�뽫��ǰЭ���л�����̨
			raw_ptr->swapOut();
		}
	}

	void IOManager::onTimerInsertedAtFront()
	{
		tickle();
	}

	//�����ļ��������ﾳ������С
	void IOManager::resizeFile_descriptor_contexts(const size_t size)
	{
		m_file_descriptor_contexts.resize(size);

		for (size_t i = 0; i < m_file_descriptor_contexts.size(); ++i)
		{
			if (!m_file_descriptor_contexts[i])
			{
				m_file_descriptor_contexts[i] = new FileDescriptorContext;
				m_file_descriptor_contexts[i]->m_file_descriptor = i;
			}
		}
	}
}
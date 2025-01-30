#include "concurrent/iomanager.h"
#include <sys/epoll.h>
#include <fcntl.h>

namespace IOManagerSpace
{
	// ���¼�����ע���¼���ɾ����������֮
	void IOManager::FileDescriptorEvent::triggerEvent(const EventType event_type)
	{
		// Ҫ�������¼�ӦΪ��ע���¼������򱨴�
		if (!(m_registered_event_types & event_type))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		// ����ע���¼���ɾ��event
		m_registered_event_types = (EventType)(m_registered_event_types & ~event_type);

		auto &task = getTask(event_type);
		if (task.m_fiber)
		{
			GetThis()->schedule(task.m_fiber);
		}
	}

	// class IOManager:public
	IOManager::IOManager(size_t thread_count, const bool use_caller, const string &name)
		: Scheduler(thread_count, use_caller, name)
	{
		// ����epoll�ļ�������
		m_epoll_file_descriptor = epoll_create(5000); // �°汾��linuxϵͳ��epoll_create()��size�����Ѿ�û�������ˣ�ֻҪ��֤����0����
		// ��epoll_create()�����ɹ���m_epoll_file_descriptorӦ������0�����򱨴�
		if (m_epoll_file_descriptor <= 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		// pipe()�����ܵ����ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴�
		if (pipe(m_pipe_file_descriptors) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		// ���ù�עͨ�Źܵ��Ķ�ȡ�˵�epoll�¼���Ĭ�����ӵ�epoll�ļ���������
		epoll_event epollevent;
		// ���epollevent����
		memset(&epollevent, 0, sizeof(epoll_event));
		// ��event����Ϊ�ɶ�����Ե����ģʽ��ֻ����״̬�����仯��ʱ��Żἤ��֪ͨ�����ظ�֪ͨ��
		epollevent.events = EPOLLIN | EPOLLET;
		// ��עͨ�Źܵ��Ķ�ȡ��
		epollevent.data.fd = m_pipe_file_descriptors[0];

		// ��ͨ�Źܵ��Ķ�ȡ������Ϊ������ģʽ���ɹ�����0��ʧ�ܷ���-1��ʧ���򱨴�
		if (fcntl(m_pipe_file_descriptors[0], F_SETFL, O_NONBLOCK) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		// ��ͨ�Źܵ��Ķ�ȡ�˵�event�¼����ӵ�epoll�ļ��������У��ɹ�����0��ʧ�ܷ���-1��ʧ���򱨴�
		if (epoll_ctl(m_epoll_file_descriptor, EPOLL_CTL_ADD, m_pipe_file_descriptors[0], &epollevent) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		// �����ļ��������¼�������ʼ��С
		resizeFile_descriptor_events(32);

		// ��ʼ����ʱ��������
		m_timer_manager.reset(new TimerManager());

		// IO�����߶��󴴽���Ϻ�Ĭ������
		start();
	}

	IOManager::~IOManager()
	{
		// IO�����߶���ʼ����ʱ��Ĭ��ֹͣ������
		stop();

		// �ر��ļ�������
		close(m_epoll_file_descriptor);
		close(m_pipe_file_descriptors[0]);
		close(m_pipe_file_descriptors[1]);
	}

	// �����¼����ļ���������
	bool IOManager::addEvent(const int file_descriptor, const EventType event_type, function<void()> callback)
	{
		// ����ļ��������¼�������С���㣬���Ƚ���������
		if (m_file_descriptor_events.size() <= file_descriptor)
		{
			// �ȼ��ӻ�����������m_file_descriptor_events
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
			// ÿ�ν���С���䵽����ֵ��1.5��
			resizeFile_descriptor_events(file_descriptor * 1.5);
		}

		// �ļ��������¼�
		shared_ptr<FileDescriptorEvent> file_descriptor_event;
		// ���ļ���������Ӧ���¼���������ȡ��
		{
			// �ȼ��ӻ�����������m_file_descriptor_events
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			file_descriptor_event = m_file_descriptor_events[file_descriptor];
		}

		// �ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_event->m_mutex);
		// Ҫ������¼���Ӧ���ѱ��ļ�������ע����¼������򱨴�
		if (file_descriptor_event->m_registered_event_types & event_type)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			stringstream message_sstream;
			message_sstream << "addEvent error,file_descriptor=" << file_descriptor << " event_type=" << event_type << " file_descriptor_event.m_registered_event_types=" << file_descriptor_event->m_registered_event_types;
			Assert(log_event, message_sstream.str());
		}

		// �����룺����ļ��������¼��Ѿ�ע����¼���Ϊ�գ���ִ���޸��¼�������ִ�������¼�
		int operation_code = file_descriptor_event->m_registered_event_types ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

		// ����epoll�¼�
		epoll_event epollevent;
		// ��epollevent����Ϊ��Ե����ģʽ����������ע����¼�
		epollevent.events = EPOLLET | file_descriptor_event->m_registered_event_types | event_type;
		// ���ļ��������¼�����ָ������epollevent��data_ptr��
		epollevent.data.ptr = file_descriptor_event.get();

		// ����epoll���ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴���addEvent()����false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
									<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		// ��ǰ�ȴ�ִ�е��¼�������һ
		++m_pending_event_count;

		// ����event����ע���¼��У�ע���ڿ���epoll֮��ִ�У���ȷ��epoll�����ѳɹ���
		file_descriptor_event->m_registered_event_types = (EventType)(file_descriptor_event->m_registered_event_types | event_type);

		auto &file_descriptor_task = file_descriptor_event->getTask(event_type);

		// �������Ļص�������Ϊ�գ����������ļ��������¼���Ӧ�Ļص�����
		if (callback)
		{
			shared_ptr<Fiber> fiber(new Fiber(callback));
			file_descriptor_task.m_fiber = fiber;
		}
		// �������Ļص�����Ϊ�գ��򽫵�ǰЭ�������ļ��������¼�������
		else
		{
			file_descriptor_task.m_fiber = Fiber::GetThis();
			if (file_descriptor_task.m_fiber->getState() != Fiber::EXECUTE)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				Assert(log_event);
			}
		}

		// addEvent()��������ִ�У�����0
		return true;
	}

	// ɾ���ļ��������ϵĶ�Ӧ�¼�
	bool IOManager::deleteEvent(const int file_descriptor, const EventType event_type)
	{
		// �ļ��������¼�
		shared_ptr<FileDescriptorEvent> file_descriptor_event;
		{
			// �ȼ��ӻ�����������m_file_descriptor_events
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			// ���file_descriptor�������ļ��������¼������Ĵ�С����˵��û������ļ�������ƥ����¼�������false
			if ((int)m_file_descriptor_events.size() <= file_descriptor)
			{
				return false;
			}

			// �����ļ��������¼�����ȡ��
			file_descriptor_event = m_file_descriptor_events[file_descriptor];
		}

		// �ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_event->m_mutex);

		// Ҫɾ�����¼�Ӧ���ѱ��ļ�������ע����¼������򷵻�false
		if (!(file_descriptor_event->m_registered_event_types & event_type))
		{
			return false;
		}

		// �����룺����ļ��������Ѿ�ע����¼���Ϊ�գ���ִ���޸��¼�������ִ��ɾ���¼�
		int operation_code = file_descriptor_event->m_registered_event_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

		// ����epoll�¼�
		epoll_event epollevent;
		// ��epollevent����Ϊ��Ե����ģʽ����������ע����¼�
		epollevent.events = EPOLLET | file_descriptor_event->m_registered_event_types & ~event_type;
		// ���ļ��������¼�����ָ������epollevent��data_ptr��
		epollevent.data.ptr = file_descriptor_event.get();

		// ����epoll���ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴���deleteEvent()����false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
									<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		// ��ǰ�ȴ�ִ�е��¼�������һ
		--m_pending_event_count;

		// ����ע���¼���ɾ��event
		file_descriptor_event->m_registered_event_types = (EventType)(file_descriptor_event->m_registered_event_types & ~event_type);

		////�����õ���ʽ��ȡ�ļ��������¼��еĻص�����
		// auto& callback = file_descriptor_event->getCallback(event_type);
		////���ûص������ÿ�
		// callback = nullptr;

		auto &file_descriptor_task = file_descriptor_event->getTask(event_type); // new
		file_descriptor_task.reset();											 // new

		// deleteEvent()��������ִ�У�����true
		return true;
	}

	// ���㣨������ɾ�����ļ��������ϵĶ�Ӧ�¼�
	bool IOManager::settleEvent(const int file_descriptor, const EventType event_type)
	{
		// �ļ��������¼�
		shared_ptr<FileDescriptorEvent> file_descriptor_context;
		{
			// �ȼ��ӻ�����������
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			// ���file_descriptor�������ļ��������¼������Ĵ�С����˵��û������ļ�������ƥ����¼�������false
			if ((int)m_file_descriptor_events.size() <= file_descriptor)
			{
				return false;
			}

			// �����ļ��������¼�����ȡ��
			file_descriptor_context = m_file_descriptor_events[file_descriptor];
		}

		// �ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		// Ҫɾ��ȡ�����¼�Ӧ���ѱ��ļ�������ע����¼������򷵻�false
		if (!(file_descriptor_context->m_registered_event_types & event_type))
		{
			return false;
		}

		// �����룺����ļ��������Ѿ�ע����¼���Ϊ�գ���ִ���޸��¼�������ִ��ɾ���¼�
		int operation_code = file_descriptor_context->m_registered_event_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

		// ����epoll�¼�
		epoll_event epollevent;
		// ��epollevent����Ϊ��Ե����ģʽ����������ע����¼�
		epollevent.events = EPOLLET | file_descriptor_context->m_registered_event_types & ~event_type;
		// ���ļ��������¼�����ָ������epollevent��data_ptr��
		epollevent.data.ptr = file_descriptor_context.get();

		// ����epoll���ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴���deleteEvent()����false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
									<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		// ���¼�����ע���¼���ɾ����������֮
		file_descriptor_context->triggerEvent(event_type);

		// ��ǰ�ȴ�ִ�е��¼�������һ
		--m_pending_event_count;

		// cancelEvent()��������ִ�У�����true
		return true;
	}

	// ���㣨������ɾ�����ļ��������ϵ������¼�
	bool IOManager::settleAllEvents(const int file_descriptor)
	{
		// �ļ��������¼�
		shared_ptr<FileDescriptorEvent> file_descriptor_context;
		{
			// �ȼ��ӻ�����������
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			// ���file_descriptor�������ļ��������¼������Ĵ�С����˵��û������ļ�������ƥ����¼�������false
			if ((int)m_file_descriptor_events.size() <= file_descriptor)
			{
				return false;
			}

			// �����ļ��������¼�����ȡ��
			file_descriptor_context = m_file_descriptor_events[file_descriptor];
		}

		// �ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		// Ҫɾ��ȫ���¼����ļ��������¼�����ע���¼���ӦΪ�գ����򷵻�false
		if (!file_descriptor_context->m_registered_event_types)
		{
			return false;
		}

		// �����룺ִ��ɾ���¼�
		int operation_code = EPOLL_CTL_DEL;

		// ����epoll�¼�
		epoll_event epollevent;
		// ��epollevent����Ϊ��
		epollevent.events = NONE;
		// ���ļ��������¼�����ָ������epollevent��data_ptr��
		epollevent.data.ptr = file_descriptor_context.get();

		// ����epoll���ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴���cancelAllEvent()����false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			// shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
									<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		// ������ע��READ�¼����������ע���¼���ɾ����������֮
		if (file_descriptor_context->m_registered_event_types & READ)
		{
			file_descriptor_context->triggerEvent(READ);
			// ��ǰ�ȴ�ִ�е��¼�������һ
			--m_pending_event_count;
		}
		// ������ע��WRITE�¼����������ע���¼���ɾ����������֮
		if (file_descriptor_context->m_registered_event_types & WRITE)
		{
			file_descriptor_context->triggerEvent(WRITE);
			// ��ǰ�ȴ�ִ�е��¼�������һ
			--m_pending_event_count;
		}

		// ������������ע���¼�ӦΪ�գ����򱨴�
		if (file_descriptor_context->m_registered_event_types != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			Assert(log_event);
		}

		// cancelAllEvent()��������ִ�У�����true
		return true;
	}

	// ���Ӷ�ʱ����������Ҫʱ֪ͨ������������
	void IOManager::addTimer(const shared_ptr<Timer> timer)
	{
		// ����¼���Ķ�ʱ��λ�ڶ�ʱ�����ϵĿ�ͷ��˵����Ҫ֪ͨ��������������
		if (m_timer_manager->addTimer(timer))
		{
			tickle();
		}
	}

	// class IOManager:protected
	// ֪ͨ��������������
	void IOManager::tickle()
	{
		// �����п����߳�ʱ�ŷ�����Ϣ
		if (getIdle_thread_count() > 0)
		{
			// ��"T"д��ͨ�Źܵ���д���
			int return_value = write(m_pipe_file_descriptors[1], "T", 1);
			// ���write()ʧ���򱨴�
			if (return_value != 1)
			{
				// shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				Assert(log_event);
			}
		}
	}
	// �����Ƿ񿢹�
	bool IOManager::isCompleted()
	{
		// ������Scheduler::is_completed()��ǰ���£���Ӧ�����㵱ǰ�ȴ�ִ�е��¼�����Ϊ0����ʱû����һ����ʱ�����ܿ���
		return !m_timer_manager->hasTimer() && m_pending_event_count == 0 && Scheduler::isCompleted();
	}
	// ����Э�̵Ļص�����
	void IOManager::idle()
	{
		// epoll�¼�����
		epoll_event *epollevents = new epoll_event[64]();
		// ������ָ���epoll�¼�����Ķ�̬�ڴ���й���
		shared_ptr<epoll_event> shared_events(epollevents, [](epoll_event *ptr)
											  { delete[] ptr; });

		// ��ѯepoll�¼��Ͷ�ʱ��
		while (true)
		{
			// ����Ѿ����������˳�idleЭ�̵���ѯѭ��������idleЭ��
			if (isCompleted())
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "name=" << getName() << " idle exit";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
				break;
			}

			// ���ó�ʱʱ�䣨��̬������Ϊ3000ms
			static const int MAX_TIMEOUT = 3000;

			// epoll_wait�ĵȴ�ʱ��
			uint64_t epoll_wait_time = m_timer_manager->getTimeUntilNextTimer();
			// ���������һ����ʱ��ִ�л���Ҫ��ʱ�䲻���������epoll_wait�ĵȴ�ʱ��ȡ���볬ʱʱ��Ľ�Сֵ
			if (epoll_wait_time != ~0ull)
			{
				epoll_wait_time = epoll_wait_time < MAX_TIMEOUT ? epoll_wait_time : MAX_TIMEOUT;
			}
			// ����epoll_wait�ȴ�ʱ������Ϊ��ʱʱ��
			else
			{
				epoll_wait_time = MAX_TIMEOUT;
			}

			// ������epollevent����
			int epollevent_count = 0;
			// ��������epoll_wait()�ȴ��¼���ֱ����epoll�¼����������߷������жϵĴ���
			while (true)
			{
				// epoll_wait()���ؾ������ļ�����������,������������򷵻�-1
				// ���ȴ�64��epoll�¼�����õȴ�next_timeout����ʱ����0
				epollevent_count = epoll_wait(m_epoll_file_descriptor, epollevents, 64, epoll_wait_time);

				// �����epoll�¼�������epoll_wait()�ȴ���ʱ�������жϵĴ������˳�ѭ��
				if (epollevent_count >= 0 || errno != EINTR)
				{
					break;
				}
			}

			// ��ȡ���ڵģ���Ҫִ�еģ���ʱ���Ļص������б�
			vector<function<void()>> expired_callbacks;
			m_timer_manager->getExpired_callbacks(expired_callbacks);
			// ������Щ�ص�����
			if (!expired_callbacks.empty())
			{
				schedule(expired_callbacks.begin(), expired_callbacks.end());
				expired_callbacks.clear();
			}

			// ���δ���������epoll�¼�
			for (size_t i = 0; i < epollevent_count; ++i)
			{
				// �����õ���ʽ��ȡepoll�¼�
				epoll_event &epollevent = epollevents[i];

				// �����epoll�¼���ע��ͨ�Źܵ��Ķ�ȡ�ˣ���ֻ���ȡ����գ�������������ݣ���������ѭ��
				if (epollevent.data.fd == m_pipe_file_descriptors[0])
				{
					uint8_t dummy = 0;
					while (read(m_pipe_file_descriptors[0], &dummy, 1) == 1)
						;

					continue;
				}

				// �ļ��������¼�
				FileDescriptorEvent *file_descriptor_event = (FileDescriptorEvent *)epollevent.data.ptr;
				// �ȼ��ӻ�����������
				ScopedLock<Mutex> lock(file_descriptor_event->m_mutex);

				// ���epoll�¼���������������������Ѿ�׼���ã���epoll�¼�����Ϊ����������������
				/*if (epollevent.events & (EPOLLERR | EPOLLOUT))
				{
					epollevent.events |= EPOLLIN | EPOLLOUT;
				}*/
				if (epollevent.events & (EPOLLERR | EPOLLHUP))
				{
					epollevent.events |= (EPOLLIN | EPOLLOUT) & file_descriptor_event->m_registered_event_types; //???
				}

				// ʵ��Ҫִ�е��¼�
				int real_events = NONE;
				if (epollevent.events & EPOLLIN)
				{
					real_events |= READ;
				}
				if (epollevent.events & EPOLLOUT)
				{
					real_events |= WRITE;
				}

				// ���ʵ��Ҫִ�е��¼���δ���ļ��������ﾳע�ᣬ��������ѭ��
				if ((file_descriptor_event->m_registered_event_types & real_events) == NONE)
				{
					continue;
				}

				// �ļ��������ﾳ��ʵ��Ҫִ�е��¼����⣬ʣ�����ע���¼�
				int left_events = (file_descriptor_event->m_registered_event_types & ~real_events);

				// �����룺���ʣ����ע���¼��¼���Ϊ�գ���ִ���޸��¼�������ִ��ɾ���¼�
				int operation_code = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
				// ����epoll�¼�
				epollevent.events = EPOLLET | left_events;

				// ����epoll���ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴�����������ѭ��
				int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor_event->m_file_descriptor, &epollevent);
				if (return_value != 0)
				{
					// shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
					shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
					log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code
											<< "," << file_descriptor_event->m_file_descriptor << "," << epollevent.events << "):"
											<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
					Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
					continue;
				}

				// ����ʵ��Ҫִ�е�READ�¼�������֮
				if (real_events & READ)
				{
					file_descriptor_event->triggerEvent(READ);
					// ��ǰ�ȴ�ִ�е��¼�������һ
					--m_pending_event_count;
				}
				// ����ʵ��Ҫִ�е�WRITE�¼�������֮
				if (real_events & WRITE)
				{
					file_descriptor_event->triggerEvent(WRITE);
					// ��ǰ�ȴ�ִ�е��¼�������һ
					--m_pending_event_count;
				}
			}

			// ��ȡ��ǰЭ��
			shared_ptr<Fiber> current = Fiber::GetThis();
			// ���л�����̨֮ǰ������ָ���л�Ϊ��ָ�룬��ֹshared_ptr�ļ�������������������������
			auto raw_ptr = current.get();
			// ���shared_ptr
			current.reset();
			// ʹ����ָ�뽫��ǰЭ���л�����̨
			raw_ptr->swapOut();
		}
	}

	// �����ļ��������¼�������С
	void IOManager::resizeFile_descriptor_events(const size_t size)
	{
		// ����������С
		m_file_descriptor_events.resize(size);

		for (size_t i = 0; i < m_file_descriptor_events.size(); ++i)
		{
			// �����λ��Ԫ��Ϊ�գ�Ϊ������ڴ沢���ļ�����������ΪԪ���±�
			if (!m_file_descriptor_events[i])
			{
				m_file_descriptor_events[i].reset(new FileDescriptorEvent());
				m_file_descriptor_events[i]->m_file_descriptor = i;
			}
		}
	}
}
#include "iomanager.h"
#include <sys/epoll.h>
#include <fcntl.h>

namespace IOManagerSpace
{
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

	void IOManager::FileDescriptorContext::resetContext(EventContext& event_context)
	{
		event_context.m_scheduler = nullptr;
		event_context.m_fiber.reset();
		event_context.m_callback = nullptr;
	}

	void IOManager::FileDescriptorContext::triggerEvent(Event event)
	{
		if (!(m_events & event))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}
		m_events = (Event)(m_events & ~event);
		EventContext& event_context = getContext(event);
		if (event_context.m_callback)
		{
			//event_context.m_scheduler->schedule(&event_context.m_callback);
			event_context.m_scheduler->schedule(event_context.m_callback);
		}
		else
		{
			//event_context.m_scheduler->schedule(&event_context.m_fiber);
			event_context.m_scheduler->schedule(event_context.m_fiber);
		}
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
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		
		//pipe()�����ܵ����ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴�
		if (pipe(m_tickle_file_descriptors) != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		epoll_event event;
		//���event����
		memset(&event, 0, sizeof(epoll_event));
		//��event����Ϊ�ɶ�����Ե����ģʽ��ֻ����״̬�����仯��ʱ��Żἤ��֪ͨ�����ظ�֪ͨ��
		event.events = EPOLLIN | EPOLLET;
		event.data.fd = m_tickle_file_descriptors[0];

		//�������ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴�
		if (fcntl(m_tickle_file_descriptors[0], F_SETFL, O_NONBLOCK) != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		
		//�������ɹ�����0��ʧ�ܷ���-1������ʧ���򱨴�
		if (epoll_ctl(m_epoll_file_descriptor, EPOLL_CTL_ADD, m_tickle_file_descriptors[0], &event) != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//m_file_descriptor_contexts.resize(64);
		contextResize(32);

		//IO�����߶��󴴽���Ϻ�Ĭ������
		start();
	}
	IOManager::~IOManager()
	{
		stop();
		close(m_epoll_file_descriptor);
		close(m_tickle_file_descriptors[0]);
		close(m_tickle_file_descriptors[1]);

		for (size_t i = 0; i < m_file_descriptor_contexts.size(); ++i)
		{
			if (m_file_descriptor_contexts[i])
			{
				delete m_file_descriptor_contexts[i];
			}
		}
	}

	//1:success	0:retry	-1:error
	int IOManager::addEvent(const int file_descriptor, const Event event, function<void()> callback)
	{
		FileDescriptorContext* file_descriptor_context;
		//�ȼ��ӻ�����������
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

		if ((int)m_file_descriptor_contexts.size() > file_descriptor)
		{
			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
			readlock.unlock();
		}
		else
		{
			readlock.unlock();
			//�ȼ��ӻ�����������
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
			//ÿ�ν���С���䵽1.5��
			contextResize(file_descriptor * 1.5);
			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);
		//Ҫ������¼���Ӧ���ļ�������������¼���ͬ�����򱨴�
		if (file_descriptor_context->m_events & event)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			stringstream message_sstream;
			message_sstream << "addEvent error,file_descriptor=" << file_descriptor <<
				" event=" << event << " file_descriptor_context.event=" << file_descriptor_context->m_events;
			Assert(log_event,message_sstream.str());
		}

		int op = file_descriptor_context->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
		epoll_event epollevent;
		epollevent.events = EPOLLET | file_descriptor_context->m_events | event;
		epollevent.data.ptr = file_descriptor_context;

		int return_value = epoll_ctl(m_epoll_file_descriptor, op, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << op << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return -1;
		}

		++m_pending_event_count;
		file_descriptor_context->m_events = (Event)(file_descriptor_context->m_events | event);
		FileDescriptorContext::EventContext& event_context = file_descriptor_context->getContext(event);

		if (event_context.m_callback || event_context.m_fiber || event_context.m_scheduler)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		event_context.m_scheduler = Scheduler::t_scheduler;
		if (callback)
		{
			event_context.m_callback.swap(callback);
		}
		else
		{
			event_context.m_fiber = Fiber::GetThis();
			if (event_context.m_fiber->getState() != Fiber::EXECUTE)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}
		}
		return 0;
	}


	//ɾ���¼�
	bool IOManager::deleteEvent(const int file_descriptor, const Event event)
	{
		FileDescriptorContext* file_descriptor_context = nullptr;
		{
			//�ȼ��ӻ�����������
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			if ((int)m_file_descriptor_contexts.size() <= file_descriptor)
			{
				return false;
			}

			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//�ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		if (!(file_descriptor_context->m_events & event))
		{
			return false;
		}
		
		Event new_events = (Event)(file_descriptor_context->m_events & ~event);
		int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
		epoll_event epollevent;
		epollevent.events = EPOLLET | new_events;
		epollevent.data.ptr = file_descriptor_context;

		int return_value = epoll_ctl(m_epoll_file_descriptor, op, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << op << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		--m_pending_event_count;
		file_descriptor_context->m_events = new_events;
		FileDescriptorContext::EventContext& event_context = file_descriptor_context->getContext(event);
		file_descriptor_context->resetContext(event_context);
		return true;
	}

	//ȡ���¼�
	bool IOManager::cancelEvent(const int file_descriptor, const Event event)
	{
		FileDescriptorContext* file_descriptor_context = nullptr;
		{
			//�ȼ��ӻ�����������
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			if ((int)m_file_descriptor_contexts.size() <= file_descriptor)
			{
				return false;
			}

			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//�ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		if (!(file_descriptor_context->m_events & event))
		{
			return false;
		}

		Event new_events = (Event)(file_descriptor_context->m_events & ~event);
		int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
		epoll_event epollevent;
		epollevent.events = EPOLLET | new_events;
		epollevent.data.ptr = file_descriptor_context;

		int return_value = epoll_ctl(m_epoll_file_descriptor, op, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << op << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}


		file_descriptor_context->triggerEvent(event);
		--m_pending_event_count;
		return true;
	}

	bool IOManager::cancelAllEvents(const int file_descriptor)
	{
		FileDescriptorContext* file_descriptor_context = nullptr;
		{
			//�ȼ��ӻ�����������
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			if ((int)m_file_descriptor_contexts.size() <= file_descriptor)
			{
				return false;
			}

			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//�ȼ��ӻ�����������
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		if (!file_descriptor_context->m_events)
		{
			return false;
		}

		
		int op = EPOLL_CTL_DEL;
		epoll_event epollevent;
		epollevent.events = 0;
		epollevent.data.ptr = file_descriptor_context;

		int return_value = epoll_ctl(m_epoll_file_descriptor, op, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << op << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		if (file_descriptor_context->m_events & READ)
		{
			file_descriptor_context->triggerEvent(READ);
			--m_pending_event_count;
		}
		if (file_descriptor_context->m_events & WRITE)
		{
			file_descriptor_context->triggerEvent(WRITE);
			--m_pending_event_count;
		}

		if (file_descriptor_context->m_events != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}
		return true;
	}

	IOManager* IOManager::GetThis()
	{
		return dynamic_cast<IOManager*>(Scheduler::t_scheduler);
	}

	void IOManager::tickle()
	{
		//�����п����߳�ʱ�ŷ�����Ϣ
		if (getIdle_thread_count() > 0)
		{
			int return_value = write(m_tickle_file_descriptors[1], "T", 1);
			if (return_value != 1)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(log_event);
			}
		}
	}

	bool IOManager::is_completed()
	{
		return Scheduler::is_completed() && m_pending_event_count == 0;
	}
	void IOManager::idle()
	{
		epoll_event* events = new epoll_event[64]();
		shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) {delete[] ptr; });


		while (true)
		{
			if (is_completed())
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "name=" << getName() << " idle exit";
				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

				break;
			}

			int return_value = 0;
			do
			{
				static const int MAX_TIMEOUT = 5000;
				return_value = epoll_wait(m_epoll_file_descriptor,events, 64, MAX_TIMEOUT);
				if (return_value >=0 || errno != EINTR)
				{
					break;
				}
			} while (true);

			for (size_t i = 0; i < return_value; ++i)
			{
				epoll_event& event = events[i];
				if (event.data.fd == m_tickle_file_descriptors[0])
				{
					uint8_t dummy = 0;
					while (read(m_tickle_file_descriptors[0], &dummy, 1) == 1);

					continue;
				}

				FileDescriptorContext* file_descriptor_context =(FileDescriptorContext*)event.data.ptr;
				//�ȼ��ӻ�����������
				ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);
				if (event.events & (EPOLLERR | EPOLLOUT))
				{
					event.events |= EPOLLIN | EPOLLOUT;
				}
				int real_events = NONE;
				if (event.events & EPOLLIN)
				{
					real_events |= READ;
				}
				if (event.events & EPOLLOUT)
				{
					real_events |= WRITE;
				}

				if ((file_descriptor_context->m_events & real_events) == NONE)
				{
					continue;
				}

				int left_events = (file_descriptor_context->m_events & ~real_events);
				int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
				event.events = EPOLLET | left_events;
				int rt2 = epoll_ctl(m_epoll_file_descriptor, op, file_descriptor_context->m_file_descriptor, &event);
				if (rt2)
				{
					shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
					log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << op
						<< "," << file_descriptor_context->m_file_descriptor << "," << event.events << "):"
						<< rt2 << " (" << errno << ") (" << strerror(errno) << ")";
					Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
					continue;
				}

				if (real_events & READ)
				{
					file_descriptor_context->triggerEvent(READ);
					--m_pending_event_count;
				}
				if (real_events & WRITE)
				{
					file_descriptor_context->triggerEvent(WRITE);
					--m_pending_event_count;
				}
			}

			shared_ptr<Fiber> current = Fiber::GetThis();
			auto raw_ptr = current.get();
			current.reset();

			raw_ptr->swapOut();
		}
	}

	void IOManager::contextResize(const size_t size)
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
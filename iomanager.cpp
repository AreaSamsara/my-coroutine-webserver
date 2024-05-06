#include "iomanager.h"
#include <sys/epoll.h>
#include <fcntl.h>

namespace IOManagerSpace
{
	//获取事件对应的语境
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

	//重置语境
	void IOManager::FileDescriptorContext::resetContext(EventContext& event_context)
	{
		event_context.m_scheduler = nullptr;
		event_context.m_fiber.reset();
		event_context.m_callback = nullptr;
	}

	//触发事件
	void IOManager::FileDescriptorContext::triggerEvent(Event event)
	{
		//要触发的事件应为已注册事件，否则报错
		if (!(m_events & event))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		//从已注册事件中删除event
		m_events = (Event)(m_events & ~event);

		//以引用的形式获取文件描述符语境中的事件语境
		EventContext& event_context = getContext(event);

		//如果事件语境有回调函数，用其调用调度方法
		if (event_context.m_callback)
		{
			//event_context.m_scheduler->schedule(&event_context.m_callback);
			event_context.m_scheduler->schedule(event_context.m_callback);
		}
		//否则用事件语境的协程来调用调度方法
		else
		{
			//event_context.m_scheduler->schedule(&event_context.m_fiber);
			event_context.m_scheduler->schedule(event_context.m_fiber);
		}

		//重置事件语境的调度器
		event_context.m_scheduler = nullptr;
	}

	IOManager::IOManager(size_t thread_count, const bool use_caller, const string& name)
		:Scheduler(thread_count,use_caller,name)
	{
		//设置epoll文件描述符
		m_epoll_file_descriptor = epoll_create(5000);	//新版本的linux系统中epoll_create()的size参数已经没有意义了，只要保证大于0即可
		//若epoll_create()创建成功，m_epoll_file_descriptor应当大于0，否则报错
		if (m_epoll_file_descriptor <= 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		
		//pipe()创建管道，成功返回0，失败返回-1；创建失败则报错
		if (pipe(m_pipe_file_descriptors) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		//设置关注通信管道的读取端的epoll事件，默认添加到epoll文件描述符中
		epoll_event epollevent;
		//清空epollevent变量
		memset(&epollevent, 0, sizeof(epoll_event));
		//将event设置为可读、边缘触发模式（只有在状态发生变化的时候才会激活通知，不重复通知）
		epollevent.events = EPOLLIN | EPOLLET;
		//关注通信管道的读取端
		epollevent.data.fd = m_pipe_file_descriptors[0];

		//将通信管道的读取端设置为非阻塞模式，成功返回0，失败返回-1；失败则报错
		if (fcntl(m_pipe_file_descriptors[0], F_SETFL, O_NONBLOCK) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}
		
		//将通信管道的读取端的event事件添加到epoll文件描述符中，成功返回0，失败返回-1；失败则报错
		if (epoll_ctl(m_epoll_file_descriptor, EPOLL_CTL_ADD, m_pipe_file_descriptors[0], &epollevent) != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		//设置文件描述符语境容器大小
		resizeFile_descriptor_contexts(32);

		//IO管理者对象创建完毕后，默认启动
		start();
	}

	IOManager::~IOManager()
	{
		//IO管理者对象开始析构时，默认停止调度器
		stop();

		//关闭文件描述符
		close(m_epoll_file_descriptor);
		close(m_pipe_file_descriptors[0]);
		close(m_pipe_file_descriptors[1]);

		//清空文件描述符语境容器
		for (size_t i = 0; i < m_file_descriptor_contexts.size(); ++i)
		{
			if (m_file_descriptor_contexts[i])
			{
				delete m_file_descriptor_contexts[i];
			}
		}
	}

	//添加事件，成功返回0，失败返回-1
	int IOManager::addEvent(const int file_descriptor, const Event event, function<void()> callback)
	{
		//文件描述符语境
		FileDescriptorContext* file_descriptor_context = nullptr;
		
		{
			////先监视互斥锁，保护
			//ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			////如果文件描述符语境容器大小足够，则将文件描述符语境从中取出
			//if (m_file_descriptor_contexts.size() > file_descriptor)
			//{
			//	file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
			//	//readlock.unlock();
			//}
			////否则先扩充容器再取出
			//else
			//{
			//	//先解锁读取锁，再设置写入锁
			//	readlock.unlock();
			//	//先监视互斥锁，保护
			//	WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
			//	//每次将大小扩充到需求值的1.5倍
			//	resizeFile_descriptor_contexts(file_descriptor * 1.5);
			//	file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
			//}
		}
		//如果文件描述符语境容器大小不足，则先将容器扩充
		if (m_file_descriptor_contexts.size() <= file_descriptor)
		{
			//先监视互斥锁，保护
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
			//每次将大小扩充到需求值的1.5倍
			resizeFile_descriptor_contexts(file_descriptor * 1.5);
		}
		{
			//先监视互斥锁，保护
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
			//将文件描述符语境从容器中取出
			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//先监视互斥锁，保护
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);
		//要加入的事件不应是已被文件描述符注册的事件，否则报错
		if (file_descriptor_context->m_events & event)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			stringstream message_sstream;
			message_sstream << "addEvent error,file_descriptor=" << file_descriptor <<
				" event=" << event << " file_descriptor_context.event=" << file_descriptor_context->m_events;
			Assert(log_event,message_sstream.str());
		}

		//操作码：如果文件描述符语境已经注册的事件不为空，则执行修改事件；否则执行添加事件
		int operation_code = file_descriptor_context->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

		//设置epoll事件
		epoll_event epollevent;
		//将epollevent设置为边缘触发模式，并设置已注册的事件
		epollevent.events = EPOLLET | file_descriptor_context->m_events | event;
		epollevent.data.ptr = file_descriptor_context;

		//控制epoll，成功返回0，失败返回-1；创建失败则报错且addEvent()返回-1
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return -1;
		}



		//当前等待执行的事件数量加一
		++m_pending_event_count;

		//添加event到已注册事件中（注册在控制epoll之后执行，以确保epoll控制已成功）
		file_descriptor_context->m_events = (Event)(file_descriptor_context->m_events | event);


		//以引用的形式获取文件描述符语境中的事件语境
		FileDescriptorContext::EventContext& event_context = file_descriptor_context->getContext(event);

		//event_context应为空，否则报错
		if (event_context.m_callback || event_context.m_fiber || event_context.m_scheduler)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		//设置事件语境的调度器为当前调度器
		event_context.m_scheduler = Scheduler::t_scheduler;
		//如果传入了回调函数，用其设置事件语境的回调函数
		if (callback)
		{
			event_context.m_callback.swap(callback);
		}
		//否则将事件语境的协程设置为当前协程
		else
		{
			event_context.m_fiber = Fiber::GetThis();
			//事件语境的协程应当处于执行状态，否则报错
			if (event_context.m_fiber->getState() != Fiber::EXECUTE)
			{
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(event);
			}
		}
		
		//addEvent()函数正常执行，返回0
		return 0;
	}


	//删除事件
	bool IOManager::deleteEvent(const int file_descriptor, const Event event)
	{
		//文件描述符语境
		FileDescriptorContext* file_descriptor_context = nullptr;
		{
			//先监视互斥锁，保护
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			//如果file_descriptor超出了文件描述符语境容器的大小，则说明没有与该文件描述符匹配的事件，返回false
			if ((int)m_file_descriptor_contexts.size() <= file_descriptor)
			{
				return false;
			}

			//否则将文件描述符语境从中取出
			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//先监视互斥锁，保护
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		//要删除的事件应是已被文件描述符注册的事件，否则返回false
		if (!(file_descriptor_context->m_events & event))
		{
			return false;
		}
		
		//Event new_events = (Event)(file_descriptor_context->m_events & ~event);

		//操作码：如果文件描述符已经注册的事件不为空，则执行修改事件；否则执行删除事件
		//int operation_code = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
		int operation_code = file_descriptor_context->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

		//设置epoll事件
		epoll_event epollevent;
		//epollevent.events = EPOLLET | new_events;
		epollevent.events = EPOLLET | file_descriptor_context->m_events & ~event;
		epollevent.data.ptr = file_descriptor_context;

		//控制epoll，成功返回0，失败返回-1；创建失败则报错且deleteEvent()返回false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}



		//当前等待执行的事件数量减一
		--m_pending_event_count;

		//从已注册事件中删除event
		//file_descriptor_context->m_events = new_events;
		file_descriptor_context->m_events = (Event)(file_descriptor_context->m_events & ~event);

		//以引用的形式获取文件描述符语境中的事件语境
		FileDescriptorContext::EventContext& event_context = file_descriptor_context->getContext(event);
		//重置该语境
		file_descriptor_context->resetContext(event_context);

		//deleteEvent()函数正常执行，返回true
		return true;
	}

	//取消事件
	bool IOManager::cancelEvent(const int file_descriptor, const Event event)
	{
		//文件描述符语境
		FileDescriptorContext* file_descriptor_context = nullptr;
		{
			//先监视互斥锁，保护
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			//如果file_descriptor超出了文件描述符语境容器的大小，则说明没有与该文件描述符匹配的事件，返回false
			if ((int)m_file_descriptor_contexts.size() <= file_descriptor)
			{
				return false;
			}

			//否则将文件描述符语境从中取出
			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//先监视互斥锁，保护
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		//要删除的事件应是已被文件描述符注册的事件，否则返回false
		if (!(file_descriptor_context->m_events & event))
		{
			return false;
		}

		//Event new_events = (Event)(file_descriptor_context->m_events & ~event);

		//操作码：如果文件描述符已经注册的事件不为空，则执行修改事件；否则执行删除事件
		//int operation_code = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
		int operation_code = file_descriptor_context->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

		//设置epoll事件
		epoll_event epollevent;
		//epollevent.events = EPOLLET | new_events;
		epollevent.events = EPOLLET | file_descriptor_context->m_events & ~event;
		epollevent.data.ptr = file_descriptor_context;

		//控制epoll，成功返回0，失败返回-1；创建失败则报错且deleteEvent()返回false
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

		//当前等待执行的事件数量减一
		--m_pending_event_count;

		//cancelEvent()函数正常执行，返回true
		return true;
	}

	//取消所有事件
	bool IOManager::cancelAllEvents(const int file_descriptor)
	{
		//文件描述符语境
		FileDescriptorContext* file_descriptor_context = nullptr;
		{
			//先监视互斥锁，保护
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			//如果file_descriptor超出了文件描述符语境容器的大小，则说明没有与该文件描述符匹配的事件，返回false
			if ((int)m_file_descriptor_contexts.size() <= file_descriptor)
			{
				return false;
			}

			//否则将文件描述符语境从中取出
			file_descriptor_context = m_file_descriptor_contexts[file_descriptor];
		}

		//先监视互斥锁，保护
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		//要删除全部事件的文件描述符语境的已注册事件不应为空，否则返回false
		if (!file_descriptor_context->m_events)
		{
			return false;
		}

		//操作码：执行删除事件
		int operation_code = EPOLL_CTL_DEL;

		//设置epoll事件
		epoll_event epollevent;
		epollevent.events = 0;	//设置为空
		epollevent.data.ptr = file_descriptor_context;

		//控制epoll，成功返回0，失败返回-1；创建失败则报错且cancelAllEvent()返回false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		//根据已注册READ事件，触发之
		if (file_descriptor_context->m_events & READ)
		{
			file_descriptor_context->triggerEvent(READ);
			//当前等待执行的事件数量减一
			--m_pending_event_count;
		}
		//根据已注册WRITE事件，触发之
		if (file_descriptor_context->m_events & WRITE)
		{
			file_descriptor_context->triggerEvent(WRITE);
			//当前等待执行的事件数量减一
			--m_pending_event_count;
		}

		//触发结束后已注册事件应为空，否则报错
		if (file_descriptor_context->m_events != 0)
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}

		//cancelAllEvent()函数正常执行，返回true
		return true;
	}

	IOManager* IOManager::GetThis()
	{
		return dynamic_cast<IOManager*>(Scheduler::t_scheduler);
	}

	//通知调度器有任务了
	void IOManager::tickle()
	{
		//仅在有空闲线程时才发送消息
		if (getIdle_thread_count() > 0)
		{
			//将"T"写入通信管道的写入端
			int return_value = write(m_pipe_file_descriptors[1], "T", 1);
			//如果write()失败则报错
			if (return_value != 1)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(log_event);
			}
		}
	}
	////返回是否竣工
	//bool IOManager::is_completed()
	//{
	//	//在满足Scheduler::is_completed()的前提下，当前等待执行的事件数量也应为0才能竣工
	//	return m_pending_event_count == 0 && Scheduler::is_completed();
	//}
	//返回是否竣工
	bool IOManager::is_completed()
	{
		//在满足Scheduler::is_completed()的前提下，当前等待执行的事件数量也应为0才能竣工
		uint64_t timeout = 0;
		return is_completed(timeout);
	}
	//返回是否竣工
	bool IOManager::is_completed(uint64_t & timeout)
	{
		timeout = getNextTimer();
		return timeout == ~0ull && m_pending_event_count == 0 && Scheduler::is_completed();
	}


	//空闲协程的回调函数
	void IOManager::idle()
	{
		epoll_event* epollevents = new epoll_event[64]();
		shared_ptr<epoll_event> shared_events(epollevents, [](epoll_event* ptr) {delete[] ptr; });

		while (true)
		{
			uint64_t next_timeout = 0;
			//如果已经竣工，则退出循环
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
			//持续调用epoll_wait()等待事件，直到有epoll事件就绪，或者发生非中断的错误
			do
			{
				//设置超时时间（静态变量）为3000ms
				static const int MAX_TIMEOUT = 3000;

				if (next_timeout != ~0ull)
				{
					next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
				}
				else
				{
					next_timeout = MAX_TIMEOUT;
				}

				//epoll_wait()返回就绪的文件描述符数量,如果发生错误则返回-1
				//最多等待64个epoll事件，最久等待next_timeout，超时返回0
				return_value = epoll_wait(m_epoll_file_descriptor,epollevents, 64, next_timeout);

				//如果有epoll事件就绪、epoll_wait()等待超时或发生非中断的错误，则退出循环
				if (return_value >=0 || errno != EINTR)
				{
					break;
				}
			} while (true);

			vector<function<void()>> callbacks;
			//获取需要执行的定时器的回调函数列表
			listExpireCallback(callbacks);
			//调度这些回调函数
			if (!callbacks.empty())
			{
				schedule(callbacks.begin(), callbacks.end());
				/*for (auto& callback : callbacks)
				{
					schedule(callback);
				}*/
				callbacks.clear();
			}

			//依次处理就绪的epoll事件
			for (size_t i = 0; i < return_value; ++i)
			{
				//以引用的形式获取epoll事件
				epoll_event& epollevent = epollevents[i];

				//如果该epoll事件关注了通信管道的读取端，则只需读取（清空）上面的所有数据，结束本次循环
				if (epollevent.data.fd == m_pipe_file_descriptors[0])
				{
					uint8_t dummy = 0;
					while (read(m_pipe_file_descriptors[0], &dummy, 1) == 1);

					continue;
				}

				//文件描述符语境
				FileDescriptorContext* file_descriptor_context =(FileDescriptorContext*)epollevent.data.ptr;
				//先监视互斥锁，保护
				ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

				//如果epoll事件发生错误且输出缓冲区已经准备好，则将epoll事件设置为输入就绪且输出就绪
				if (epollevent.events & (EPOLLERR | EPOLLOUT))
				{
					epollevent.events |= EPOLLIN | EPOLLOUT;
				}

				//实际要执行的事件
				int real_events = NONE;
				if (epollevent.events & EPOLLIN)
				{
					real_events |= READ;
				}
				if (epollevent.events & EPOLLOUT)
				{
					real_events |= WRITE;
				}

				//如果实际要执行的事件均未被文件描述符语境注册，结束本次循环
				if ((file_descriptor_context->m_events & real_events) == NONE)
				{
					continue;
				}

				//文件描述符语境除实际要执行的事件以外，剩余的已注册事件
				int left_events = (file_descriptor_context->m_events & ~real_events);

				//操作码：如果剩余已注册事件事件不为空，则执行修改事件；否则执行删除事件
				int operation_code = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
				//设置epoll事件
				epollevent.events = EPOLLET | left_events;


				//控制epoll，成功返回0，失败返回-1；创建失败则报错并结束本次循环
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

				//根据实际要执行的READ事件，触发之
				if (real_events & READ)
				{
					file_descriptor_context->triggerEvent(READ);
					//当前等待执行的事件数量减一
					--m_pending_event_count;
				}
				//根据实际要执行的WRITE事件，触发之
				if (real_events & WRITE)
				{
					file_descriptor_context->triggerEvent(WRITE);
					//当前等待执行的事件数量减一
					--m_pending_event_count;
				}
			}

			//获取当前协程
			shared_ptr<Fiber> current = Fiber::GetThis();
			//在切换到后台之前将智能指针切换为裸指针，防止shared_ptr的计数错误导致析构函数不被调用
			auto raw_ptr = current.get();
			//清空shared_ptr
			current.reset();
			//使用裸指针将当前协程切换到后台
			raw_ptr->swapOut();
		}
	}

	void IOManager::onTimerInsertedAtFront()
	{
		tickle();
	}

	//重置文件描述符语境容器大小
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
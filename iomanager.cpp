#include "iomanager.h"
#include <sys/epoll.h>
#include <fcntl.h>

namespace IOManagerSpace
{
	//将事件从已注册事件中删除，并触发之
	void IOManager::FileDescriptorEvent::triggerEvent(const EventType event_type)
	{
		//要触发的事件应为已注册事件，否则报错
		if (!(m_registered_event_types & event_type))
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		//从已注册事件中删除event
		m_registered_event_types = (EventType)(m_registered_event_types & ~event_type);

		auto& task = getTask(event_type);
		if (task.m_fiber)
		{
			GetThis()->schedule(task.m_fiber);
		}
	}



	//class IOManager:public
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

		//设置文件描述符事件容器初始大小
		resizeFile_descriptor_events(32);


		//初始化定时器管理者
		m_timer_manager.reset(new TimerManager());

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
	}

	//添加事件到文件描述符上
	bool IOManager::addEvent(const int file_descriptor, const EventType event_type, function<void()> callback)
	{
		//如果文件描述符事件容器大小不足，则先将容器扩充
		if (m_file_descriptor_events.size() <= file_descriptor)
		{
			//先监视互斥锁，保护m_file_descriptor_events
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
			//每次将大小扩充到需求值的1.5倍
			resizeFile_descriptor_events(file_descriptor * 1.5);
		}

		//文件描述符事件
		shared_ptr<FileDescriptorEvent> file_descriptor_event;
		//将文件描述符对应的事件从容器中取出
		{
			//先监视互斥锁，保护m_file_descriptor_events
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			file_descriptor_event = m_file_descriptor_events[file_descriptor];
		}

		//先监视互斥锁，保护
		ScopedLock<Mutex> lock(file_descriptor_event->m_mutex);
		//要加入的事件不应是已被文件描述符注册的事件，否则报错
		if (file_descriptor_event->m_registered_event_types & event_type)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			stringstream message_sstream;
			message_sstream << "addEvent error,file_descriptor=" << file_descriptor <<
				" event_type=" << event_type << " file_descriptor_event.m_registered_event_types=" << file_descriptor_event->m_registered_event_types;
			Assert(log_event,message_sstream.str());
		}



		//操作码：如果文件描述符事件已经注册的事件不为空，则执行修改事件；否则执行添加事件
		int operation_code = file_descriptor_event->m_registered_event_types ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

		//设置epoll事件
		epoll_event epollevent;
		//将epollevent设置为边缘触发模式，并添加已注册的事件
		epollevent.events = EPOLLET | file_descriptor_event->m_registered_event_types | event_type;
		//将文件描述符事件的裸指针存放在epollevent的data_ptr中
		epollevent.data.ptr = file_descriptor_event.get();

		//控制epoll，成功返回0，失败返回-1；控制失败则报错且addEvent()返回false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}



		//当前等待执行的事件数量加一
		++m_pending_event_count;

		//添加event到已注册事件中（注册在控制epoll之后执行，以确保epoll控制已成功）
		file_descriptor_event->m_registered_event_types = (EventType)(file_descriptor_event->m_registered_event_types | event_type);

		////以引用的形式获取文件描述符事件中的回调函数
		//auto& file_descriptor_event_callback = file_descriptor_event->getCallback(event_type);

		////文件描述符事件的回调函数应为空，否则报错
		//if (file_descriptor_event_callback)
		//{
		//	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		//	Assert(log_event);
		//}

		////将文件描述符事件的回调函数设置为传入的回调函数
		//file_descriptor_event_callback.swap(callback);

		auto& file_descriptor_task = file_descriptor_event->getTask(event_type);

		//如果传入的回调函数不为空，则将其设作文件描述符事件对应的回调函数
		if (callback)
		{
			shared_ptr<Fiber> fiber(new Fiber(callback));
			file_descriptor_task.m_fiber = fiber;
		}
		//如果传入的回调函数为空，则将当前协程设作文件描述符事件的任务
		else
		{
			file_descriptor_task.m_fiber = Fiber::GetThis();
			if (file_descriptor_task.m_fiber->getState() != Fiber::EXECUTE)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				Assert(log_event);
			}
		}

		//addEvent()函数正常执行，返回0
		return true;
	}

	//删除文件描述符上的对应事件
	bool IOManager::deleteEvent(const int file_descriptor, const EventType event_type)
	{
		//文件描述符事件
		shared_ptr<FileDescriptorEvent> file_descriptor_event;
		{
			//先监视互斥锁，保护m_file_descriptor_events
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			//如果file_descriptor超出了文件描述符事件容器的大小，则说明没有与该文件描述符匹配的事件，返回false
			if ((int)m_file_descriptor_events.size() <= file_descriptor)
			{
				return false;
			}

			//否则将文件描述符事件从中取出
			file_descriptor_event = m_file_descriptor_events[file_descriptor];
		}

		//先监视互斥锁，保护
		ScopedLock<Mutex> lock(file_descriptor_event->m_mutex);

		//要删除的事件应是已被文件描述符注册的事件，否则返回false
		if (!(file_descriptor_event->m_registered_event_types & event_type))
		{
			return false;
		}

		//操作码：如果文件描述符已经注册的事件不为空，则执行修改事件；否则执行删除事件
		int operation_code = file_descriptor_event->m_registered_event_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

		//设置epoll事件
		epoll_event epollevent;
		//将epollevent设置为边缘触发模式，并添加已注册的事件
		epollevent.events = EPOLLET | file_descriptor_event->m_registered_event_types & ~event_type;
		//将文件描述符事件的裸指针存放在epollevent的data_ptr中
		epollevent.data.ptr = file_descriptor_event.get();

		//控制epoll，成功返回0，失败返回-1；控制失败则报错且deleteEvent()返回false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}



		//当前等待执行的事件数量减一
		--m_pending_event_count;

		//从已注册事件中删除event
		file_descriptor_event->m_registered_event_types = (EventType)(file_descriptor_event->m_registered_event_types & ~event_type);

		////以引用的形式获取文件描述符事件中的回调函数
		//auto& callback = file_descriptor_event->getCallback(event_type);
		////将该回调函数置空
		//callback = nullptr;

		auto& file_descriptor_task = file_descriptor_event->getTask(event_type);	//new
		file_descriptor_task.reset();	//new


		//deleteEvent()函数正常执行，返回true
		return true;
	}

	//结算（触发并删除）文件描述符上的对应事件
	bool IOManager::settleEvent(const int file_descriptor, const EventType event_type)
	{
		//文件描述符事件
		shared_ptr<FileDescriptorEvent> file_descriptor_context;
		{
			//先监视互斥锁，保护
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			//如果file_descriptor超出了文件描述符事件容器的大小，则说明没有与该文件描述符匹配的事件，返回false
			if ((int)m_file_descriptor_events.size() <= file_descriptor)
			{
				return false;
			}

			//否则将文件描述符事件从中取出
			file_descriptor_context = m_file_descriptor_events[file_descriptor];
		}

		//先监视互斥锁，保护
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		//要删除取消的事件应是已被文件描述符注册的事件，否则返回false
		if (!(file_descriptor_context->m_registered_event_types & event_type))
		{
			return false;
		}

		//操作码：如果文件描述符已经注册的事件不为空，则执行修改事件；否则执行删除事件
		int operation_code = file_descriptor_context->m_registered_event_types ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

		//设置epoll事件
		epoll_event epollevent;
		//将epollevent设置为边缘触发模式，并添加已注册的事件
		epollevent.events = EPOLLET | file_descriptor_context->m_registered_event_types & ~event_type;
		//将文件描述符事件的裸指针存放在epollevent的data_ptr中
		epollevent.data.ptr = file_descriptor_context.get();

		//控制epoll，成功返回0，失败返回-1；控制失败则报错且deleteEvent()返回false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		//将事件从已注册事件中删除，并触发之
		file_descriptor_context->triggerEvent(event_type);

		//当前等待执行的事件数量减一
		--m_pending_event_count;

		//cancelEvent()函数正常执行，返回true
		return true;
	}

	//结算（触发并删除）文件描述符上的所有事件
	bool IOManager::settleAllEvents(const int file_descriptor)
	{
		//文件描述符事件
		shared_ptr<FileDescriptorEvent> file_descriptor_context;
		{
			//先监视互斥锁，保护
			ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);

			//如果file_descriptor超出了文件描述符事件容器的大小，则说明没有与该文件描述符匹配的事件，返回false
			if ((int)m_file_descriptor_events.size() <= file_descriptor)
			{
				return false;
			}

			//否则将文件描述符事件从中取出
			file_descriptor_context = m_file_descriptor_events[file_descriptor];
		}

		//先监视互斥锁，保护
		ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

		//要删除全部事件的文件描述符事件的已注册事件不应为空，否则返回false
		if (!file_descriptor_context->m_registered_event_types)
		{
			return false;
		}

		//操作码：执行删除事件
		int operation_code = EPOLL_CTL_DEL;

		//设置epoll事件
		epoll_event epollevent;
		//将epollevent设置为空
		epollevent.events = NONE;
		//将文件描述符事件的裸指针存放在epollevent的data_ptr中
		epollevent.data.ptr = file_descriptor_context.get();

		//控制epoll，成功返回0，失败返回-1；控制失败则报错且cancelAllEvent()返回false
		int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor, &epollevent);
		if (return_value)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code << "," << file_descriptor << "," << epollevent.events << "):"
				<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		//根据已注册READ事件，将其从已注册事件中删除，并触发之
		if (file_descriptor_context->m_registered_event_types & READ)
		{
			file_descriptor_context->triggerEvent(READ);
			//当前等待执行的事件数量减一
			--m_pending_event_count;
		}
		//根据已注册WRITE事件，将其从已注册事件中删除，并触发之
		if (file_descriptor_context->m_registered_event_types & WRITE)
		{
			file_descriptor_context->triggerEvent(WRITE);
			//当前等待执行的事件数量减一
			--m_pending_event_count;
		}

		//触发结束后已注册事件应为空，否则报错
		if (file_descriptor_context->m_registered_event_types != 0)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(log_event);
		}

		//cancelAllEvent()函数正常执行，返回true
		return true;
	}

	//添加定时器，并在需要时通知调度器有任务
	void IOManager::addTimer(const shared_ptr<Timer> timer)
	{
		//如果新加入的定时器位于定时器集合的开头，说明需要通知调度器有任务了
		if (m_timer_manager->addTimer(timer))
		{
			tickle();
		}
	}



	//class IOManager:protected
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
	//返回是否竣工
	bool IOManager::isCompleted()
	{
		//在满足Scheduler::is_completed()的前提下，还应当满足当前等待执行的事件数量为0、暂时没有下一个定时器才能竣工
		return !m_timer_manager->hasTimer() && m_pending_event_count == 0 && Scheduler::isCompleted();
	}
	//空闲协程的回调函数
	void IOManager::idle()
	{
		//epoll事件数组
		epoll_event* epollevents = new epoll_event[64]();
		//用智能指针对epoll事件数组的动态内存进行管理
		shared_ptr<epoll_event> shared_events(epollevents, [](epoll_event* ptr) {delete[] ptr; });

		//轮询epoll事件和定时器
		while (true)
		{
			//如果已经竣工，则退出idle协程的轮询循环，结束idle协程
			if (isCompleted())
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "name=" << getName() << " idle exit";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
				break;
			}

			//就绪的epollevent数量
			//int epollevent_count = 0;
			
			//设置超时时间（静态变量）为3000ms
			static const int MAX_TIMEOUT = 3000;

			//epoll_wait的等待时间
			uint64_t epoll_wait_time = m_timer_manager->getTimeUntilNextTimer();
			//如果距离下一个定时器执行还需要的时间不是无穷大，则epoll_wait的等待时间取其与超时时间的较小值
			if (epoll_wait_time != ~0ull)
			{
				epoll_wait_time = epoll_wait_time < MAX_TIMEOUT ? epoll_wait_time : MAX_TIMEOUT;
			}
			//否则将epoll_wait等待时间设置为超时时间
			else
			{
				epoll_wait_time = MAX_TIMEOUT;
			}


			//就绪的epollevent数量
			int epollevent_count = 0;
			//持续调用epoll_wait()等待事件，直到有epoll事件就绪，或者发生非中断的错误
			while(true)
			{
				//epoll_wait()返回就绪的文件描述符数量,如果发生错误则返回-1
				//最多等待64个epoll事件，最久等待next_timeout，超时返回0
				epollevent_count = epoll_wait(m_epoll_file_descriptor, epollevents, 64, epoll_wait_time);

				//如果有epoll事件就绪、epoll_wait()等待超时或发生非中断的错误，则退出循环
				if (epollevent_count >=0 || errno != EINTR)
				{
					break;
				}
			}


			//获取到期的（需要执行的）定时器的回调函数列表
			vector<function<void()>> expired_callbacks;
			m_timer_manager->getExpired_callbacks(expired_callbacks);
			//调度这些回调函数
			if (!expired_callbacks.empty())
			{
				schedule(expired_callbacks.begin(), expired_callbacks.end());
				expired_callbacks.clear();
			}


			//依次处理就绪的epoll事件
			for (size_t i = 0; i < epollevent_count; ++i)
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
				FileDescriptorEvent* file_descriptor_context =(FileDescriptorEvent*)epollevent.data.ptr;
				//先监视互斥锁，保护
				ScopedLock<Mutex> lock(file_descriptor_context->m_mutex);

				//如果epoll事件发生错误且输出缓冲区已经准备好，则将epoll事件设置为输入就绪且输出就绪
				/*if (epollevent.events & (EPOLLERR | EPOLLOUT))
				{
					epollevent.events |= EPOLLIN | EPOLLOUT;
				}*/
				if (epollevent.events & (EPOLLERR | EPOLLHUP)) 
				{
					epollevent.events |= (EPOLLIN | EPOLLOUT) & file_descriptor_context->m_registered_event_types;	//???
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
				if ((file_descriptor_context->m_registered_event_types & real_events) == NONE)
				{
					continue;
				}

				//文件描述符语境除实际要执行的事件以外，剩余的已注册事件
				int left_events = (file_descriptor_context->m_registered_event_types & ~real_events);

				//操作码：如果剩余已注册事件事件不为空，则执行修改事件；否则执行删除事件
				int operation_code = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
				//设置epoll事件
				epollevent.events = EPOLLET | left_events;


				//控制epoll，成功返回0，失败返回-1；创建失败则报错并结束本次循环
				int return_value = epoll_ctl(m_epoll_file_descriptor, operation_code, file_descriptor_context->m_file_descriptor, &epollevent);
				if (return_value)
				{
					shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
					log_event->getSstream() << "epoll_ctl(" << m_epoll_file_descriptor << "," << operation_code
						<< "," << file_descriptor_context->m_file_descriptor << "," << epollevent.events << "):"
						<< return_value << " (" << errno << ") (" << strerror(errno) << ")";
					Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
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

	//重设文件描述符事件容器大小
	void IOManager::resizeFile_descriptor_events(const size_t size)
	{
		//重设容器大小
		m_file_descriptor_events.resize(size);

		for (size_t i = 0; i < m_file_descriptor_events.size(); ++i)
		{
			//如果该位置元素为空，为其分配内存并将文件描述符设置为元素下标
			if (!m_file_descriptor_events[i])
			{
				m_file_descriptor_events[i].reset(new FileDescriptorEvent());
				m_file_descriptor_events[i]->m_file_descriptor = i;
			}
		}
	}
}
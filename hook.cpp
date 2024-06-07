#include "hook.h"
#include <dlfcn.h>
#include <stdarg.h>

#include "iomanager.h"
#include "fdmanager.h"


//千万不要把包含函数定义的extern "C"模块放入namespace HookSpace，否则本文件乃至所有新文件的语法检查功能将直接停摆（原因未知）
namespace HookSpace
{
	using namespace IOManagerSpace;
	using namespace FdManagerSpace;
	using std::forward;

	//tcp连接超时时间
	uint64_t s_tcp_connect_timeout = 5000;
	//线程专属静态变量，表示当前线程是否hook住（是否启用hook）
	thread_local bool t_hook_enable = false;

	//进行hook版本IO操作的通用函数
	template<typename OriginFunction, typename ... Args>
	static ssize_t do_io(int fd, OriginFunction function_origin, const char* hook_function_name, uint32_t event, int timeout_type, Args&&...args)
	{
		//如果没有被hook住，直接执行原始的库方法
		if (!t_hook_enable)
		{
			return function_origin(fd, forward<Args>(args)...);
		}

		//获取socket对应的文件描述符实体
		//shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_shared_ptr()->getFile_descriptor(fd);
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(fd);	//此处有不可言状的bug，在main函数结束时会调用两次write函数，会导致shared_ptr计数出错，故应该使用裸指针

		//如果文件描述符实体不存在，说明肯定不是socket文件描述符，直接执行原始的库方法
		if (!file_descriptor_entity)
		{
			return function_origin(fd, forward<Args>(args)...);
		}
		//如果文件描述符已经被关闭，将错误代码设置为"bad file descriptor"，并返回-1
		else if (file_descriptor_entity->isClose())
		{
			errno = EBADF;
			return -1;
		}
		//如果文件描述符不是socket,或者用户已经主动将其设置为非阻塞，直接执行原始的库方法
		else if (!file_descriptor_entity->isSocket() || file_descriptor_entity->isUserNonblock())
		{
			return function_origin(fd, forward<Args>(args)...);
		}


		//超时时间
		uint64_t timeout = file_descriptor_entity->getTimeout(timeout_type);
		//标志定时器是否取消的shared_ptr
		shared_ptr<int> timer_cancelled(new int);
		//将timer_cancelled初始化为0，表示未取消
		*timer_cancelled = 0;

		//操作执行循环
		while (true)
		{
			//尝试直接执行原始的库方法
			ssize_t return_value = function_origin(fd, forward<Args>(args)...);
			//如果返回值为-1且错误类型为中断，则不断尝试
			while (return_value == -1 && errno == EINTR)
			{
				return_value = function_origin(fd, forward<Args>(args)...);
			}
			//否则如果返回值为-1且错误类型为资源暂时不可用，则进行异步操作
			if (return_value == -1 && errno == EAGAIN)
			{
				//获取当前线程的IO管理者
				IOManager* iomanager = IOManager::GetThis();
				//定时器
				shared_ptr<Timer> timer;
				//标志定时器是否取消的weak_ptr，即定时器超时的条件
				weak_ptr<int> timer_cancelled_weak(timer_cancelled);

				//如果超时时间不等于-1，说明设置了超时时间
				if (timeout != (uint64_t)-1)
				{
					//设置定时器，如果到时间时条件未失效，则结算事件
					timer.reset(new Timer(timeout, [timer_cancelled_weak, fd, iomanager, event]()
						{
							auto condition = timer_cancelled_weak.lock();
							//如果条件已经失效（do_io已因被epoll系统处理而返回），直接返回
							if (!condition)
							{
								return;
							}
							//否则设置错误码并结算协程事件，唤醒do_io的当前协程
							else
							{
								//设置操作超时错误码，表示在socket可用之前事件已超时
								*condition = ETIMEDOUT;
								//结算该socket的事件（即为将do_io当前协程唤醒，见后文）
								iomanager->settleEvent(fd, IOManager::EventType(event));
							}
						}
					, timer_cancelled_weak));
					//添加定时器
					iomanager->addTimer(timer);
				}

				//不为事件设置回调函数，默认使用当前协程为回调参数
				bool return_value = iomanager->addEvent(fd, IOManager::EventType(event), nullptr);
				//如果添加失败，报错并返回-1
				if (return_value == false)
				{
					shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
					log_event->getSstream() << hook_function_name << " addEvent(" << fd << ", " << event << ")";
					Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);

					//如果定时器不为空，取消之
					if (timer)
					{
						iomanager->getTimer_manager()->cancelTimer(timer);
					}
					return -1;
				}
				//否则添加成功
				else
				{
					//挂起当前协程，等待定时器唤醒或epoll处理事件（即socket已经可用）时被唤醒
					Fiber::YieldToHold();

					//如果定时器不为空，取消之
					if (timer)
					{
						iomanager->getTimer_manager()->cancelTimer(timer);
					}
					//如果是通过定时器任务唤醒的，设置错误代码并返回-1
					if (*timer_cancelled!=0)
					{
						errno = *timer_cancelled;
						return -1;
					}
					//否则说明当前协程不是因为超时而是因为socket可用被唤醒，重新尝试执行循环
				}
			}
			//如果出现其他错误或者返回值不为-1，则do_io()返回该返回值
			else
			{
				return return_value;
			}
		}
	}
}

//按照C风格编译,声明原始库函数，并重写其hook版本
extern "C"
{
	using namespace HookSpace;

	//sleep相关
	unsigned int (*sleep_origin)(unsigned int) = (unsigned int (*)(unsigned int))dlsym(RTLD_NEXT, "sleep");
	int (*usleep_origin)(useconds_t usec) = (int (*)(useconds_t))dlsym(RTLD_NEXT, "usleep");
	int (*nanosleep_origin)(const struct timespec* req, struct timespec* rem) = (int (*)(const struct timespec* req, struct timespec* rem))dlsym(RTLD_NEXT, "nanosleep");

	//socket相关
	int (*socket_origin)(int domain, int type, int protocol) = (int (*)(int domain, int type, int protocol))dlsym(RTLD_NEXT, "socket");

	int (*connect_origin)(int sockfd, const struct sockaddr* addr, socklen_t addrlen) = (int (*)(int sockfd, const struct sockaddr* addr, socklen_t addrlen))dlsym(RTLD_NEXT, "connect");
	int (*accept_origin)(int s, struct sockaddr* addr, socklen_t* addrlen) = (int (*)(int s, struct sockaddr* addr, socklen_t * addrlen))dlsym(RTLD_NEXT, "accept");

	//read相关
	ssize_t(*read_origin)(int fd, void* buf, size_t count) = (ssize_t(*)(int fd, void* buf, size_t count))dlsym(RTLD_NEXT, "read");
	ssize_t(*readv_origin)(int fd, const struct iovec* iov, int iovcnt) = (ssize_t(*)(int fd, const struct iovec* iov, int iovcnt))dlsym(RTLD_NEXT, "readv");
	ssize_t(*recv_origin)(int sockfd, void* buf, size_t len, int flags) = (ssize_t(*)(int sockfd, void* buf, size_t len, int flags))dlsym(RTLD_NEXT, "recv");
	ssize_t(*recvfrom_origin)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) = (ssize_t(*)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t * addrlen))dlsym(RTLD_NEXT, "recvfrom");
	ssize_t(*recvmsg_origin)(int sockfd, struct msghdr* msg, int flags) = (ssize_t(*)(int sockfd, struct msghdr* msg, int flags))dlsym(RTLD_NEXT, "recvmsg");

	//write相关
	ssize_t(*write_origin)(int fd, const void* buf, size_t count) = (ssize_t(*)(int fd, const void* buf, size_t count))dlsym(RTLD_NEXT, "write");
	ssize_t(*writev_origin)(int fd, const struct iovec* iov, int iovcnt) = (ssize_t(*)(int fd, const struct iovec* iov, int iovcnt))dlsym(RTLD_NEXT, "writev");
	ssize_t(*send_origin)(int s, const void* msg, size_t len, int flags) = (ssize_t(*)(int s, const void* msg, size_t len, int flags))dlsym(RTLD_NEXT, "send");
	ssize_t(*sendto_origin)(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen) = (ssize_t(*)(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen))dlsym(RTLD_NEXT, "sendto");
	ssize_t(*sendmsg_origin)(int s, const struct msghdr* msg, int flags) = (ssize_t(*)(int s, const struct msghdr* msg, int flags))dlsym(RTLD_NEXT, "sendmsg");

	int (*close_origin)(int fd) = (int (*)(int fd))dlsym(RTLD_NEXT, "close");
	int (*fcntl_origin)(int fd, int cmd, .../* arg */) = (int (*)(int fd, int cmd, .../* arg */))dlsym(RTLD_NEXT, "fcntl");
	int (*ioctl_origin)(int d, unsigned long int request, .../* arg */) = (int (*)(int d, unsigned long int request, .../* arg */))dlsym(RTLD_NEXT, "ioctl");

	int (*getsockopt_origin)(int sockfd, int level, int optname, void* optval, socklen_t* optlen) = (int (*)(int sockfd, int level, int optname, void* optval, socklen_t * optlen))dlsym(RTLD_NEXT, "getsockopt");
	int (*setsockopt_origin)(int sockfd, int level, int optname, const void* optval, socklen_t optlen) = (int (*)(int sockfd, int level, int optname, const void* optval, socklen_t optlen))dlsym(RTLD_NEXT, "setsockopt");





	//sleep相关
	unsigned int sleep(unsigned int seconds)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return sleep_origin(seconds);
		}

		//获取当前协程和当前IO管理者
		shared_ptr<Fiber> fiber = Fiber::GetThis();
		IOManager* iomanager = IOManager::GetThis();

		//bind()最终得到的是继承自Scheduler类的IOManager::schedule
		shared_ptr<Timer> timer(new Timer(seconds * 1000, bind((void(Scheduler::*)
			(shared_ptr<Fiber>, int thread)) & IOManager::schedule, iomanager, fiber, -1)));
		//shared_ptr<Timer> timer(new Timer(seconds * 1000, [iomanager, fiber]() {iomanager->schedule(fiber); }));
		iomanager->addTimer(timer);

		//将当前协程放入定时器后切换到后台，等到定时器被唤醒后再切换回来
		Fiber::YieldToHold();
		return 0;
	}
	int usleep(useconds_t usec)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return usleep_origin(usec);
		}

		//获取当前协程和当前IO管理者
		shared_ptr<Fiber> fiber = Fiber::GetThis();
		IOManager* iomanager = IOManager::GetThis();

		//bind()最终得到的是继承自Scheduler类的IOManager::schedule
		shared_ptr<Timer> timer(new Timer(usec / 1000, bind((void(Scheduler::*)
			(shared_ptr<Fiber>, int thread)) & IOManager::schedule, iomanager, fiber, -1)));
		//shared_ptr<Timer> timer(new Timer(usec / 1000, [iomanager, fiber]() {iomanager->schedule(fiber); }));
		iomanager->addTimer(timer);

		//将当前协程放入定时器后切换到后台，等到定时器被唤醒后再切换回来
		Fiber::YieldToHold();
		return 0;
	}
	int nanosleep(const struct timespec* req, struct timespec* rem)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return nanosleep_origin(req, rem);
		}

		int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
		//获取当前协程和当前IO管理者
		shared_ptr<Fiber> fiber = Fiber::GetThis();
		IOManager* iomanager = IOManager::GetThis();

		//bind()最终得到的是继承自Scheduler类的IOManager::schedule
		shared_ptr<Timer> timer(new Timer(timeout_ms, bind((void(Scheduler::*)
			(shared_ptr<Fiber>, int thread)) & IOManager::schedule, iomanager, fiber, -1)));
		//shared_ptr<Timer> timer(new Timer(timeout_ms, [iomanager, fiber]() {iomanager->schedule(fiber); }));
		iomanager->addTimer(timer);

		//将当前协程放入定时器后切换到后台，等到定时器被唤醒后再切换回来
		Fiber::YieldToHold();
		return 0;
	}


	//socket相关
	int socket(int domain, int type, int protocol)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return socket_origin(domain, type, protocol);
		}

		//调用原始的库方法创建socket
		int file_descriptor = socket_origin(domain, type, protocol);

		//如果创建失败，返回-1（保持和原始的库方法相同的行为）
		if (file_descriptor == -1)
		{
			return file_descriptor;
		}
		//否则创建成功
		else
		{
			//调用单例文件描述符管理者创建对应的文件描述符实体
			Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(file_descriptor, true);
			//返回文件描述符
			return file_descriptor;
		}
	}
	//按照指定的超时时间尝试连接
	int connect_with_timeout(int sockfd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return connect_origin(sockfd, addr, addrlen);
		}

		//获取socket对应的文件描述符实体
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(sockfd);
		//如果得到的文件描述符实体为空或处于关闭状态
		if (!file_descriptor_entity || file_descriptor_entity->isClose())
		{
			//设置错误码为“bad file descriptor”
			errno = EBADF;
			//返回-1（保持和原始的库方法相同的行为）
			return -1;
		}
		//如果得到的文件描述符实体不是socket或被用户主动设置为非阻塞，直接调用原始的库方法
		if (!file_descriptor_entity->isSocket() || file_descriptor_entity->isUserNonblock())
		{
			return connect_origin(sockfd, addr, addrlen);
		}

		//调用原始的库方法尝试连接
		int return_value = connect_origin(sockfd, addr, addrlen);
		//如果返回0说明连接成功，直接返回0
		if (return_value == 0)
		{
			return 0;
		}
		//如果返回值不是-1（这好像不太可能）或发生了除操作未立即完成以外的错误，直接返回原始库方法的返回值
		else if (return_value != -1 || errno != EINPROGRESS)
		{
			return return_value;
		}
		//否则说明操作未立即完成，进行异步连接
		else
		{
			//获取当前IO管理者
			IOManager* iomanager = IOManager::GetThis();

			shared_ptr<Timer> timer;
			//标志定时器是否取消的shared_ptr
			shared_ptr<int> timer_cancelled(new int);
			//将timer_cancelled初始化为0，表示未取消
			*timer_cancelled = 0;
			//标志定时器是否取消的weak_ptr，即定时器超时的条件
			weak_ptr<int> timer_cancelled_weak(timer_cancelled);

			//如果设置了超时时间（超时时间不为-1）
			if (timeout_ms != (uint64_t)-1)
			{
				//设置定时器，如果到时间时条件未失效，则结算事件
				timer.reset(new Timer(timeout_ms, [timer_cancelled_weak, sockfd, iomanager]()
					{
						//解锁条件的weak_ptr
						auto condition = timer_cancelled_weak.lock();
						//如果条件已经失效（connect_with_timeout已因被epoll系统处理而返回），直接返回
						if (!condition)
						{
							return;
						}
						//否则设置错误码并结算协程事件，唤醒connect_with_timeout的当前协程
						else
						{
							//设置操作超时错误码，表示在socket可写之前事件已超时
							*condition = ETIMEDOUT;
							//结算该socket的写入事件（即为将connect_with_timeout的当前协程唤醒，见后文）
							iomanager->settleEvent(sockfd, IOManager::WRITE);
						}
					}
				, timer_cancelled_weak));
				//添加设置好的定时器到定时器管理者中
				iomanager->addTimer(timer);
			}

			//尝试将当前协程作为写入事件添加到IO管理者中（不为事件设置回调函数，默认使用当前协程为回调参数）
			bool return_value = iomanager->addEvent(sockfd, IOManager::WRITE, nullptr);
			//如果添加成功
			if (return_value == true)
			{
				//挂起当前协程，等待定时器唤醒或epoll处理写入事件（即socket已经可写）时被唤醒
				Fiber::YieldToHold();

				//当前协程被唤醒后，如果定时器不为空，取消之
				if (timer)
				{
					iomanager->getTimer_manager()->cancelTimer(timer);
				}
				//如果在socket可写之前事件已超时，说明协程被唤醒的原因是定时器超时，设置错误码并返回-1
				if (*timer_cancelled!=0)
				{
					errno = *timer_cancelled;
					return -1;
				}
			}
			//否则添加失败，取消定时器并报错
			else
			{
				//如果定时器不为空，取消之
				if (timer)
				{
					iomanager->getTimer_manager()->cancelTimer(timer);
				}

				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "connect addEvent(" << sockfd << ", WRITE)";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			}



			//查询socket连接结果
			int error = 0;
			socklen_t len = sizeof(int);
			//如果连接失败则返回-1
			if (-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len))
			{
				return -1;
			}
			//如果连接成功且没有出错，则返回0
			else if (!error)
			{
				return 0;
			}
			//如果连接成功但出错，设置错误码并返回-1
			else
			{
				errno = error;
				return -1;
			}
		}
	}
	//按照默认的tcp连接超时时间尝试连接
	int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
	{
		//按照原始名称调用的connect函数采用默认的TCP连接超时时间
		return connect_with_timeout(sockfd, addr, addrlen, s_tcp_connect_timeout);
	}
	int accept(int s, struct sockaddr* addr, socklen_t* addrlen)
	{
		int file_descriptor = do_io(s, accept_origin, "accept", IOManager::READ, SO_RCVTIMEO, addr, addrlen);
		//如果原始库函数accept()执行成功
		if (file_descriptor >= 0)
		{
			//调用单例文件描述符管理者创建对应的文件描述符实体
			Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(file_descriptor, true);
		}
		return file_descriptor;
	}

	//read相关
	ssize_t read(int fd, void* buf, size_t count)
	{
		return do_io(fd, read_origin, "read", IOManager::READ, SO_RCVTIMEO, buf, count);
	}
	ssize_t readv(int fd, const struct iovec* iov, int iovcnt)
	{
		return do_io(fd, readv_origin, "readv", IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
	}
	ssize_t recv(int sockfd, void* buf, size_t len, int flags)
	{
		return do_io(sockfd, recv_origin, "recv", IOManager::READ, SO_RCVTIMEO, buf, len, flags);
	}
	ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
	{
		return do_io(sockfd, recvfrom_origin, "recvfrom", IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
	}
	ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags)
	{
		return do_io(sockfd, recvmsg_origin, "recvmsg", IOManager::READ, SO_RCVTIMEO, msg, flags);
	}

	//write相关
	ssize_t write(int fd, const void* buf, size_t count)
	{
		return do_io(fd, write_origin, "write", IOManager::WRITE, SO_SNDTIMEO, buf, count);
	}
	ssize_t writev(int fd, const struct iovec* iov, int iovcnt)
	{
		return do_io(fd, writev_origin, "writev", IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
	}
	ssize_t send(int s, const void* msg, size_t len, int flags)
	{
		return do_io(s, send_origin, "send", IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
	}
	ssize_t sendto(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen)
	{
		return do_io(s, sendto_origin, "sendto", IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
	}
	ssize_t sendmsg(int s, const struct msghdr* msg, int flags)
	{
		return do_io(s, sendmsg_origin, "sendmsg", IOManager::WRITE, SO_SNDTIMEO, msg, flags);
	}

	//fd相关
	int close(int fd)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return close_origin(fd);
		}
		
		//获取socket对应的文件描述符实体
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(fd);
		//如果实体不为空，结算socket上面的所有事件并删除该实体
		if (file_descriptor_entity)
		{
			auto iomanager = IOManager::GetThis();
			if (iomanager)
			{
				//结算socket上面的所有事件
				iomanager->settleAllEvents(fd);
			}
			//删除socket对应的文件描述符实体
			Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->deleteFile_descriptor(fd);
		}
		//调用原始的库方法关闭socket
		return close_origin(fd);
	}
	int fcntl(int fd, int cmd, .../* arg */)
	{
		va_list va;
		va_start(va, cmd);

		switch (cmd)
		{
		case F_SETFL:
		{
			int arg = va_arg(va, int);
			va_end(va);
			shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(fd);
			if (!file_descriptor_entity || file_descriptor_entity->isClose() || !file_descriptor_entity->isSocket())
			{
				return fcntl_origin(fd, cmd, arg);
			}

			file_descriptor_entity->setUserNonblock(arg & O_NONBLOCK);
			if (file_descriptor_entity->isSystemNonblock())
			{
				arg |= O_NONBLOCK;
			}
			else
			{
				arg &= ~O_NONBLOCK;
			}
			return fcntl_origin(fd, cmd, arg);
		}
		break;
		case F_GETFL:
		{
			va_end(va);
			int arg = fcntl_origin(fd, cmd);
			shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(fd);
			if (!file_descriptor_entity || file_descriptor_entity->isClose() || !file_descriptor_entity->isSocket())
			{
				return arg;
			}
			if (file_descriptor_entity->isUserNonblock())
			{
				return arg | O_NONBLOCK;
			}
			else
			{
				return arg & ~O_NONBLOCK;
			}
		}
		break;
		case F_DUPFD:
		case F_DUPFD_CLOEXEC:
		case F_SETFD:
		case F_SETOWN:
		case F_SETSIG:
		case F_SETLEASE:
		case F_NOTIFY:
		case F_SETPIPE_SZ:
		{
			int arg = va_arg(va, int);
			va_end(va);
			return fcntl_origin(fd, cmd, arg);
		}
		break;
		case F_GETFD:
		case F_GETOWN:
		case F_GETSIG:
		case F_GETLEASE:
		case F_GETPIPE_SZ:
		{
			va_end(va);
			return fcntl_origin(fd, cmd);
		}
		break;
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK:
		{
			struct flock* arg = va_arg(va, struct flock*);
			va_end(va);
			return fcntl_origin(fd, cmd, arg);
		}
		break;
		case F_GETOWN_EX:
		case F_SETOWN_EX:
		{
			struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
			va_end(va);
			return fcntl_origin(fd, cmd, arg);
		}
		break;
		default:
			va_end(va);
			return fcntl_origin(fd, cmd);
			break;
		}
	}
	int ioctl(int d, unsigned long int request, .../* arg */)
	{
		va_list va;
		va_start(va, request);
		void* arg = va_arg(va, void*);
		va_end(va);

		//如果request是FIONBIO
		if (FIONBIO == request)
		{
			bool user_nonblock = !!*(int*)arg;
			//获取socket对应的文件描述符实体
			shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(d);
			//如果实体为空，或实体已关闭，或实体不是socket，直接调用原始的库方法
			if (!file_descriptor_entity || file_descriptor_entity->isClose() || !file_descriptor_entity->isSocket())
			{
				return ioctl_origin(d, request, arg);
			}
			//否则将用户主动设置非阻塞的值设为arg
			file_descriptor_entity->setUserNonblock(user_nonblock);
		}
		//调用原始的库方法
		return ioctl_origin(d, request, arg);
	}
	int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
	{
		//直接调用原始的库方法
		return getsockopt_origin(sockfd, level, optname, optval, optlen);
	}
	int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return setsockopt_origin(sockfd, level, optname, optval, optlen);
		}

		//如果为通用socket代码且可选项为接收超时或发送超时，获取socket对应的文件描述符实体并设置超时时间
		if (level == SOL_SOCKET && (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO))
		{
			//获取socket对应的文件描述符实体
			shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(sockfd);
			//如果创建的文件描述符实体不为空，为其设置超时时间
			if (file_descriptor_entity)
			{
				const timeval* tv = (const timeval*)optval;
				file_descriptor_entity->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
			}
		}
		//调用原始的库方法
		return setsockopt_origin(sockfd, level, optname, optval, optlen);
	}
}
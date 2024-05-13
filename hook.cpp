#include "hook.h"
#include <dlfcn.h>
#include <stdarg.h>

#include "iomanager.h"
#include "fdmanager.h"

namespace HookSpace
{
	using namespace IOManagerSpace;
	using namespace FdManagerSpace;
	using std::forward;
	
	static uint64_t s_tcp_connect_timeout = 5000;
	static thread_local bool t_hook_enable = false;	


	//应在main函数之前完成初始化
	void hook_initialize()
	{
		static bool is_initialized = false;
		if (is_initialized)
		{
			return;
		}
	}


	class _HookInitializer
	{
	public:
		_HookInitializer()
		{
			hook_initialize();
		}
	};

	static _HookInitializer s_hook_initializer;


	void set_hook_enable(const bool flag)
	{
		t_hook_enable = flag;
	}


	void hook_function(int x)
	{
		sleep(x);
		usleep(x);
	}



	struct timer_information
	{
		int cancelled = 0;
	};

	template<typename OriginFunction, typename ... Args>
	static ssize_t do_io(int fd, OriginFunction function, const char* hook_function_name, uint32_t event,
		int timeout_so, Args&&...args)
	{
		//如果没有被hook住，直接执行function
		if (!t_hook_enable)
		{
			return function(fd, forward<Args>(args)...);
		}

		//shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_shared_ptr()->getFile_descriptor(fd);
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(fd);	//此处有不可言状的bug，在main函数结束时会调用两次write函数，会导致shared_ptr计数出错，故应该使用裸指针
		
		//如果文件描述符不存在，说明肯定不是socket文件描述符，直接执行function
		if (!file_descriptor_entity)
		{
			return function(fd, forward<Args>(args)...);
		}
		//如果文件描述符已经被关闭，将错误代码设置为"bad file descriptor"，并返回-1
		else if (file_descriptor_entity->isClose())
		{
			errno = EBADF;
			return -1;
		}
		//如果文件描述符不是socket,或者用户已经主动将其设置为非阻塞，直接执行function
		else if (!file_descriptor_entity->isSocket() || file_descriptor_entity->isUserNonblock())
		{
			return function(fd, forward<Args>(args)...);
		}


		//超时时间
		uint64_t timeout = file_descriptor_entity->getTimeout(timeout_so);
		shared_ptr<timer_information> timer_info(new timer_information);

	retry:
		//尝试直接执行function
		ssize_t n = function(fd, forward<Args>(args)...);
		//如果返回值为-1且错误类型为中断，则不断尝试
		while (n == -1 && errno == EINTR)
		{
			n = function(fd, forward<Args>(args)...);
		}
		//否则如果返回值为-1且错误类型为资源暂时不可用，则进行异步操作
		if (n == -1 && errno == EAGAIN)
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "do_io<" << hook_function_name << ">";
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);

			//获取当前线程的IO管理者
			IOManager* iomanager = IOManager::GetThis();
			shared_ptr<Timer> timer;
			//条件变量
			weak_ptr<timer_information> winfo(timer_info);

			//如果超时时间不等于-1，说明设置了超时时间
			if (timeout != (uint64_t)-1)
			{
				timer.reset(new Timer(timeout, [winfo, fd, iomanager, event]()
					{
						auto condition = winfo.lock();
						//如果条件已失效或被取消，直接返回
						if (!condition || condition->cancelled)
						{
							return;
						}
						//否则设置为请求超时
						condition->cancelled = ETIMEDOUT;
						//触发并删除事件
						iomanager->settleEvent(fd, IOManager::EventType(event));
					}
				, winfo));
				//添加定时器
				iomanager->addTimer(timer);
			}

			//不为事件设置回调函数，默认使用当前协程为回调参数
			bool return_value = iomanager->addEvent(fd, IOManager::EventType(event), nullptr);
			//如果添加失败，报错并返回-1
			if (return_value == false)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
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
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
				log_event->getSstream() << "do_io<" << hook_function_name << ">";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);

				Fiber::YieldToHold();

				log_event->setSstream("");
				log_event->getSstream() << "do_io<" << hook_function_name << ">";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);

				//如果定时器不为空，取消之
				if (timer)
				{
					iomanager->getTimer_manager()->cancelTimer(timer);
				}
				//如果是通过定时器任务唤醒的，设置错误代码并返回-1
				if (timer_info->cancelled)
				{
					errno = timer_info->cancelled;
					return -1;
				}

				//否则说明有io事件，重新执行
				goto retry;
			}
		}

		//如果返回值不为-1，则do_io()返回该返回值
		return n;
	}
}

extern "C"
{
	using namespace HookSpace;

	//sleep相关
	unsigned int (*sleep_f)(unsigned int) = (unsigned int (*)(unsigned int))dlsym(RTLD_NEXT, "sleep");
	int (*usleep_f)(useconds_t usec) = (int (*)(useconds_t))dlsym(RTLD_NEXT, "usleep");
	int (*nanosleep_f)(const struct timespec* req, struct timespec* rem) = (int (*)(const struct timespec* req, struct timespec* rem))dlsym(RTLD_NEXT, "nanosleep");

	//socket相关
	int (*socket_f)(int domain, int type, int protocol) = (int (*)(int domain, int type, int protocol))dlsym(RTLD_NEXT, "socket");

	int (*connect_f)(int sockfd, const struct sockaddr* addr, socklen_t addrlen) = (int (*)(int sockfd, const struct sockaddr* addr, socklen_t addrlen))dlsym(RTLD_NEXT, "connect");
	int (*accept_f)(int s, struct sockaddr* addr, socklen_t* addrlen) = (int (*)(int s, struct sockaddr* addr, socklen_t * addrlen))dlsym(RTLD_NEXT, "accept");

	//read相关
	ssize_t(*read_f)(int fd, void* buf, size_t count) = (ssize_t(*)(int fd, void* buf, size_t count))dlsym(RTLD_NEXT, "read");
	ssize_t(*readv_f)(int fd, const struct iovec* iov, int iovcnt) = (ssize_t(*)(int fd, const struct iovec* iov, int iovcnt))dlsym(RTLD_NEXT, "readv");
	ssize_t(*recv_f)(int sockfd, void* buf, size_t len, int flags) = (ssize_t(*)(int sockfd, void* buf, size_t len, int flags))dlsym(RTLD_NEXT, "recv");
	ssize_t(*recvfrom_f)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) = (ssize_t(*)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t * addrlen))dlsym(RTLD_NEXT, "recvfrom");
	ssize_t(*recvmsg_f)(int sockfd, struct msghdr* msg, int flags) = (ssize_t(*)(int sockfd, struct msghdr* msg, int flags))dlsym(RTLD_NEXT, "recvmsg");

	//write相关
	ssize_t(*write_f)(int fd, const void* buf, size_t count) = (ssize_t(*)(int fd, const void* buf, size_t count))dlsym(RTLD_NEXT, "write");
	ssize_t(*writev_f)(int fd, const struct iovec* iov, int iovcnt) = (ssize_t(*)(int fd, const struct iovec* iov, int iovcnt))dlsym(RTLD_NEXT, "writev");
	ssize_t(*send_f)(int s, const void* msg, size_t len, int flags) = (ssize_t(*)(int s, const void* msg, size_t len, int flags))dlsym(RTLD_NEXT, "send");
	ssize_t(*sendto_f)(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen) = (ssize_t(*)(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen))dlsym(RTLD_NEXT, "sendto");
	ssize_t(*sendmsg_f)(int s, const struct msghdr* msg, int flags) = (ssize_t(*)(int s, const struct msghdr* msg, int flags))dlsym(RTLD_NEXT, "sendmsg");

	int (*close_f)(int fd) = (int (*)(int fd))dlsym(RTLD_NEXT, "close");
	int (*fcntl_f)(int fd, int cmd, .../* arg */) = (int (*)(int fd, int cmd, .../* arg */))dlsym(RTLD_NEXT, "fcntl");
	int (*ioctl_f)(int d, unsigned long int request, .../* arg */) = (int (*)(int d,unsigned long int request, .../* arg */))dlsym(RTLD_NEXT, "ioctl");

	int (*getsockopt_f)(int sockfd, int level, int optname, void* optval, socklen_t* optlen) = (int (*)(int sockfd, int level, int optname, void* optval, socklen_t * optlen))dlsym(RTLD_NEXT, "getsockopt");
	int (*setsockopt_f)(int sockfd, int level, int optname, const void* optval, socklen_t optlen) = (int (*)(int sockfd, int level, int optname, const void* optval, socklen_t optlen))dlsym(RTLD_NEXT, "setsockopt");
	







	//sleep相关
	unsigned int sleep(unsigned int seconds)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return sleep_f(seconds);
		}

		shared_ptr<Fiber> fiber = Fiber::GetThis();
		IOManager* iomanager = IOManager::GetThis();

		//bind()最终得到的是继承自Scheduler类的IOManager::schedule
		shared_ptr<Timer> timer(new Timer(seconds * 1000, bind((void(Scheduler::*)
			(shared_ptr<Fiber>, int thread)) & IOManager::schedule, iomanager, fiber, -1)));
		//shared_ptr<Timer> timer(new Timer(seconds * 1000, [iomanager, fiber]() {iomanager->schedule(fiber); }));
		iomanager->addTimer(timer);
		Fiber::YieldToHold();
		return 0;
	}

	int usleep(useconds_t usec)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return usleep_f(usec);
		}

		shared_ptr<Fiber> fiber = Fiber::GetThis();
		IOManager* iomanager = IOManager::GetThis();

		//bind()最终得到的是继承自Scheduler类的IOManager::schedule
		shared_ptr<Timer> timer(new Timer(usec / 1000, bind((void(Scheduler::*)
			(shared_ptr<Fiber>, int thread)) & IOManager::schedule, iomanager, fiber, -1)));
		//shared_ptr<Timer> timer(new Timer(usec / 1000, [iomanager, fiber]() {iomanager->schedule(fiber); }));
		iomanager->addTimer(timer);
		Fiber::YieldToHold();
		return 0;
	}

	int nanosleep(const struct timespec* req, struct timespec* rem)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return nanosleep_f(req,rem);
		}

		int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
		shared_ptr<Fiber> fiber = Fiber::GetThis();
		IOManager* iomanager = IOManager::GetThis();

		//bind()最终得到的是继承自Scheduler类的IOManager::schedule
		shared_ptr<Timer> timer(new Timer(timeout_ms, bind((void(Scheduler::*)
			(shared_ptr<Fiber>, int thread)) & IOManager::schedule, iomanager, fiber, -1)));
		//shared_ptr<Timer> timer(new Timer(timeout_ms, [iomanager, fiber]() {iomanager->schedule(fiber); }));
		iomanager->addTimer(timer);
		Fiber::YieldToHold();
		return 0;
	}


	//socket相关
	int socket(int domain, int type, int protocol)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return socket_f(domain,type,protocol);
		}

		int file_descriptor = socket_f(domain, type, protocol);
		if (file_descriptor == -1)
		{
			return file_descriptor;
		}
		
		Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(file_descriptor,true);
		return file_descriptor;
	}
	int connect_with_timeout(int sockfd, const struct sockaddr* addr, socklen_t addrlen,uint64_t timeout_ms)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return connect_f(sockfd, addr, addrlen);
		}
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(sockfd);
		if (!file_descriptor_entity || file_descriptor_entity->isClose())
		{
			errno = EBADF;
			return -1;
		}
		if (!file_descriptor_entity->isSocket())
		{
			return connect_f(sockfd, addr, addrlen);
		}

		if (file_descriptor_entity->isUserNonblock())
		{
			return connect_f(sockfd, addr, addrlen);
		}

		int n= connect_f(sockfd, addr, addrlen);
		if (n == 0)
		{
			return 0;
		}
		else if (n != -1 || errno != EINPROGRESS)
		{
			return n;
		}

		IOManager* iomanager = IOManager::GetThis();
		shared_ptr<Timer> timer;
		shared_ptr<timer_information> timer_info(new timer_information);
		weak_ptr<timer_information> winfo(timer_info);

		if (timeout_ms != (uint64_t)-1)
		{
			timer.reset(new Timer(timeout_ms, [winfo, sockfd, iomanager]()
				{
					auto condition = winfo.lock();
					if (!condition || condition->cancelled)
					{
						return;
					}
					condition->cancelled = ETIMEDOUT;
					iomanager->settleEvent(sockfd, IOManager::WRITE);
				}
			, winfo));
			iomanager->addTimer(timer);
		}

		bool return_value = iomanager->addEvent(sockfd, IOManager::WRITE,nullptr);
		if (return_value == true)
		{
			Fiber::YieldToHold();	//new
			if (timer)
			{
				iomanager->getTimer_manager()->cancelTimer(timer);
			}
			if (timer_info->cancelled)
			{
				errno = timer_info->cancelled;
				return -1;	//new
			}
		}
		else
		{
			if (timer)
			{
				iomanager->getTimer_manager()->cancelTimer(timer);
			}
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "connect addEvent(" << sockfd << ", WRITE)" ;
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
		}

		int error = 0;
		socklen_t len = sizeof(int);
		if (-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len))
		{
			return -1;
		}
		if (!error)
		{
			return 0;
		}
		else
		{
			errno = error;
			return -1;
		}
	}
	int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
	{
		return connect_with_timeout(sockfd, addr, addrlen, s_tcp_connect_timeout);
	}
	int accept(int s, struct sockaddr* addr, socklen_t* addrlen)
	{
		int file_descriptor = do_io(s, accept_f, "accept", IOManager::READ, SO_RCVTIMEO, addr, addrlen);
		if (file_descriptor >= 0)
		{
			Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(file_descriptor,true);
		}
		return file_descriptor;
	}

	//read相关
	ssize_t read(int fd, void* buf, size_t count)
	{
		return do_io(fd, read_f, "read", IOManager::READ, SO_RCVTIMEO, buf, count);
	}
	ssize_t readv(int fd, const struct iovec* iov, int iovcnt)
	{
		return do_io(fd, readv_f, "readv", IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
	}
	ssize_t recv(int sockfd, void* buf, size_t len, int flags)
	{
		return do_io(sockfd, recv_f, "recv", IOManager::READ, SO_RCVTIMEO, buf, len, flags);
	}
	ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen)
	{
		return do_io(sockfd, recvfrom_f, "recvfrom", IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
	}
	ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags)
	{
		return do_io(sockfd, recvmsg_f, "recvmsg", IOManager::READ, SO_RCVTIMEO, msg, flags);
	}

	//write相关
	ssize_t write(int fd, const void* buf, size_t count)
	{
		return do_io(fd, write_f, "write", IOManager::WRITE, SO_SNDTIMEO, buf, count);
	}
	ssize_t writev(int fd, const struct iovec* iov, int iovcnt)
	{
		return do_io(fd, writev_f, "writev", IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
	}
	ssize_t send(int s, const void* msg, size_t len, int flags)
	{
		return do_io(s, send_f, "send", IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
	}
	ssize_t sendto(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen)
	{
		return do_io(s, sendto_f, "sendto", IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
	}
	ssize_t sendmsg(int s, const struct msghdr* msg, int flags)
	{
		return do_io(s, sendmsg_f, "sendmsg", IOManager::WRITE, SO_SNDTIMEO, msg, flags);
	}

	int close(int fd)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return close_f(fd);
		}
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(fd);
		
		if (file_descriptor_entity)
		{
			auto iomanager = IOManager::GetThis();
			if (iomanager)
			{
				iomanager->settleAllEvents(fd);
			}
			Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->deleteFile_descriptor(fd);
		}
		return close_f(fd);
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
			shared_ptr<FileDescriptorEntity> file_descriptor_entity= Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(fd);
			if (!file_descriptor_entity || file_descriptor_entity->isClose() || !file_descriptor_entity->isSocket())
			{
				return fcntl_f(fd, cmd, arg);
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
			return fcntl_f(fd, cmd, arg);
		}
			break;
		case F_GETFL:
		{
			va_end(va);
			int arg = fcntl_f(fd, cmd);
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
			return fcntl_f(fd, cmd, arg);
		}
			break;
		case F_GETFD:
		case F_GETOWN:
		case F_GETSIG:
		case F_GETLEASE:
		case F_GETPIPE_SZ:
		{
			va_end(va);
			return fcntl_f(fd, cmd);
		}
			break;
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK:
		{
			struct flock* arg = va_arg(va, struct flock*);
			va_end(va);
			return fcntl_f(fd, cmd, arg);
		}
			break;
		case F_GETOWN_EX:
		case F_SETOWN_EX:
		{
			struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
			va_end(va);
			return fcntl_f(fd, cmd, arg);
		}
			break;
		default:
			va_end(va);
			return fcntl_f(fd, cmd);
			break;
		}
	}
	int ioctl(int d, unsigned long int request, .../* arg */)
	{
		va_list va;
		va_start(va, request);
		void* arg = va_arg(va, void*);
		va_end(va);

		if (FIONBIO == request)
		{
			bool user_nonblock = !!*(int*)arg;
			shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(d);
			if (!file_descriptor_entity || file_descriptor_entity->isClose() || !file_descriptor_entity->isSocket())
			{
				return ioctl_f(d, request, arg);
			}
			file_descriptor_entity->setUserNonblock(user_nonblock);
		}
		return ioctl_f(d, request, arg);
	}


	int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
	{
		return getsockopt_f(sockfd, level, optname, optval, optlen);
	}

	int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
	{
		//如果没有hook住，则直接调用原始的库方法
		if (!t_hook_enable)
		{
			return setsockopt_f(sockfd, level, optname, optval, optlen);
		}
		if (level == SOL_SOCKET)
		{
			if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
			{
				shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(sockfd);
				if (file_descriptor_entity)
				{
					const timeval* tv = (const timeval*)optval;
					file_descriptor_entity->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
				}
			}
		}
		return setsockopt_f(sockfd, level, optname, optval, optlen);
	}
}
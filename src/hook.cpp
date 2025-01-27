#include "hook.h"
#include <dlfcn.h>
#include <stdarg.h>

#include "iomanager.h"
#include "fdmanager.h"


//ǧ��Ҫ�Ѱ������������extern "C"ģ�����namespace HookSpace�������ļ������������ļ����﷨��鹦�ܽ�ֱ��ͣ�ڣ�ԭ��δ֪��
namespace HookSpace
{
	using namespace IOManagerSpace;
	using namespace FdManagerSpace;
	using std::forward;

	//tcp���ӳ�ʱʱ��
	uint64_t s_tcp_connect_timeout = 5000;
	//�߳�ר����̬��������ʾ��ǰ�߳��Ƿ�hookס���Ƿ�����hook��
	thread_local bool t_hook_enable = false;

	//����hook�汾IO������ͨ�ú���
	template<typename OriginFunction, typename ... Args>
	static ssize_t do_io(int fd, OriginFunction function_origin, const char* hook_function_name, uint32_t event, int timeout_type, Args&&...args)
	{
		//���û�б�hookס��ֱ��ִ��ԭʼ�Ŀⷽ��
		if (!t_hook_enable)
		{
			return function_origin(fd, forward<Args>(args)...);
		}

		//��ȡsocket��Ӧ���ļ�������ʵ��
		//shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_shared_ptr()->getFile_descriptor(fd);
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(fd);	//�˴��в�����״��bug����main��������ʱ���������write�������ᵼ��shared_ptr������������Ӧ��ʹ����ָ��

		//����ļ�������ʵ�岻���ڣ�˵���϶�����socket�ļ���������ֱ��ִ��ԭʼ�Ŀⷽ��
		if (!file_descriptor_entity)
		{
			return function_origin(fd, forward<Args>(args)...);
		}
		//����ļ��������Ѿ����رգ��������������Ϊ"bad file descriptor"��������-1
		else if (file_descriptor_entity->isClose())
		{
			errno = EBADF;
			return -1;
		}
		//����ļ�����������socket,�����û��Ѿ�������������Ϊ��������ֱ��ִ��ԭʼ�Ŀⷽ��
		else if (!file_descriptor_entity->isSocket() || file_descriptor_entity->isUserNonblock())
		{
			return function_origin(fd, forward<Args>(args)...);
		}


		//��ʱʱ��
		uint64_t timeout = file_descriptor_entity->getTimeout(timeout_type);
		//��־��ʱ���Ƿ�ȡ����shared_ptr
		shared_ptr<int> timer_cancelled(new int);
		//��timer_cancelled��ʼ��Ϊ0����ʾδȡ��
		*timer_cancelled = 0;

		//����ִ��ѭ��
		while (true)
		{
			//����ֱ��ִ��ԭʼ�Ŀⷽ��
			ssize_t return_value = function_origin(fd, forward<Args>(args)...);
			//�������ֵΪ-1�Ҵ�������Ϊ�жϣ��򲻶ϳ���
			while (return_value == -1 && errno == EINTR)
			{
				return_value = function_origin(fd, forward<Args>(args)...);
			}
			//�����������ֵΪ-1�Ҵ�������Ϊ��Դ��ʱ�����ã�������첽����
			if (return_value == -1 && errno == EAGAIN)
			{
				//��ȡ��ǰ�̵߳�IO������
				IOManager* iomanager = IOManager::GetThis();
				//��ʱ��
				shared_ptr<Timer> timer;
				//��־��ʱ���Ƿ�ȡ����weak_ptr������ʱ����ʱ������
				weak_ptr<int> timer_cancelled_weak(timer_cancelled);

				//�����ʱʱ�䲻����-1��˵�������˳�ʱʱ��
				if (timeout != (uint64_t)-1)
				{
					//���ö�ʱ���������ʱ��ʱ����δʧЧ��������¼�
					timer.reset(new Timer(timeout, [timer_cancelled_weak, fd, iomanager, event]()
						{
							auto condition = timer_cancelled_weak.lock();
							//��������Ѿ�ʧЧ��do_io����epollϵͳ���������أ���ֱ�ӷ���
							if (!condition)
							{
								return;
							}
							//�������ô����벢����Э���¼�������do_io�ĵ�ǰЭ��
							else
							{
								//���ò�����ʱ�����룬��ʾ��socket����֮ǰ�¼��ѳ�ʱ
								*condition = ETIMEDOUT;
								//�����socket���¼�����Ϊ��do_io��ǰЭ�̻��ѣ������ģ�
								iomanager->settleEvent(fd, IOManager::EventType(event));
							}
						}
					, timer_cancelled_weak));
					//���Ӷ�ʱ��
					iomanager->addTimer(timer);
				}

				//��Ϊ�¼����ûص�������Ĭ��ʹ�õ�ǰЭ��Ϊ�ص�����
				bool return_value = iomanager->addEvent(fd, IOManager::EventType(event), nullptr);
				//�������ʧ�ܣ�����������-1
				if (return_value == false)
				{
					shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
					log_event->getSstream() << hook_function_name << " addEvent(" << fd << ", " << event << ")";
					Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);

					//�����ʱ����Ϊ�գ�ȡ��֮
					if (timer)
					{
						iomanager->getTimer_manager()->cancelTimer(timer);
					}
					return -1;
				}
				//�������ӳɹ�
				else
				{
					//����ǰЭ�̣��ȴ���ʱ�����ѻ�epoll�����¼�����socket�Ѿ����ã�ʱ������
					Fiber::YieldToHold();

					//�����ʱ����Ϊ�գ�ȡ��֮
					if (timer)
					{
						iomanager->getTimer_manager()->cancelTimer(timer);
					}
					//�����ͨ����ʱ�������ѵģ����ô�����벢����-1
					if (*timer_cancelled!=0)
					{
						errno = *timer_cancelled;
						return -1;
					}
					//����˵����ǰЭ�̲�����Ϊ��ʱ������Ϊsocket���ñ����ѣ����³���ִ��ѭ��
				}
			}
			//�����������������߷���ֵ��Ϊ-1����do_io()���ظ÷���ֵ
			else
			{
				return return_value;
			}
		}
	}
}

//����C������,����ԭʼ�⺯��������д��hook�汾
extern "C"
{
	using namespace HookSpace;

	//sleep���
	unsigned int (*sleep_origin)(unsigned int) = (unsigned int (*)(unsigned int))dlsym(RTLD_NEXT, "sleep");
	int (*usleep_origin)(useconds_t usec) = (int (*)(useconds_t))dlsym(RTLD_NEXT, "usleep");
	int (*nanosleep_origin)(const struct timespec* req, struct timespec* rem) = (int (*)(const struct timespec* req, struct timespec* rem))dlsym(RTLD_NEXT, "nanosleep");

	//socket���
	int (*socket_origin)(int domain, int type, int protocol) = (int (*)(int domain, int type, int protocol))dlsym(RTLD_NEXT, "socket");

	int (*connect_origin)(int sockfd, const struct sockaddr* addr, socklen_t addrlen) = (int (*)(int sockfd, const struct sockaddr* addr, socklen_t addrlen))dlsym(RTLD_NEXT, "connect");
	int (*accept_origin)(int s, struct sockaddr* addr, socklen_t* addrlen) = (int (*)(int s, struct sockaddr* addr, socklen_t * addrlen))dlsym(RTLD_NEXT, "accept");

	//read���
	ssize_t(*read_origin)(int fd, void* buf, size_t count) = (ssize_t(*)(int fd, void* buf, size_t count))dlsym(RTLD_NEXT, "read");
	ssize_t(*readv_origin)(int fd, const struct iovec* iov, int iovcnt) = (ssize_t(*)(int fd, const struct iovec* iov, int iovcnt))dlsym(RTLD_NEXT, "readv");
	ssize_t(*recv_origin)(int sockfd, void* buf, size_t len, int flags) = (ssize_t(*)(int sockfd, void* buf, size_t len, int flags))dlsym(RTLD_NEXT, "recv");
	ssize_t(*recvfrom_origin)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) = (ssize_t(*)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t * addrlen))dlsym(RTLD_NEXT, "recvfrom");
	ssize_t(*recvmsg_origin)(int sockfd, struct msghdr* msg, int flags) = (ssize_t(*)(int sockfd, struct msghdr* msg, int flags))dlsym(RTLD_NEXT, "recvmsg");

	//write���
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





	//sleep���
	unsigned int sleep(unsigned int seconds)
	{
		//���û��hookס����ֱ�ӵ���ԭʼ�Ŀⷽ��
		if (!t_hook_enable)
		{
			return sleep_origin(seconds);
		}

		//��ȡ��ǰЭ�̺͵�ǰIO������
		shared_ptr<Fiber> fiber = Fiber::GetThis();
		IOManager* iomanager = IOManager::GetThis();

		//bind()���յõ����Ǽ̳���Scheduler���IOManager::schedule
		shared_ptr<Timer> timer(new Timer(seconds * 1000, bind((void(Scheduler::*)
			(shared_ptr<Fiber>, int thread)) & IOManager::schedule, iomanager, fiber, -1)));
		//shared_ptr<Timer> timer(new Timer(seconds * 1000, [iomanager, fiber]() {iomanager->schedule(fiber); }));
		iomanager->addTimer(timer);

		//����ǰЭ�̷��붨ʱ�����л�����̨���ȵ���ʱ�������Ѻ����л�����
		Fiber::YieldToHold();
		return 0;
	}
	int usleep(useconds_t usec)
	{
		//���û��hookס����ֱ�ӵ���ԭʼ�Ŀⷽ��
		if (!t_hook_enable)
		{
			return usleep_origin(usec);
		}

		//��ȡ��ǰЭ�̺͵�ǰIO������
		shared_ptr<Fiber> fiber = Fiber::GetThis();
		IOManager* iomanager = IOManager::GetThis();

		//bind()���յõ����Ǽ̳���Scheduler���IOManager::schedule
		shared_ptr<Timer> timer(new Timer(usec / 1000, bind((void(Scheduler::*)
			(shared_ptr<Fiber>, int thread)) & IOManager::schedule, iomanager, fiber, -1)));
		//shared_ptr<Timer> timer(new Timer(usec / 1000, [iomanager, fiber]() {iomanager->schedule(fiber); }));
		iomanager->addTimer(timer);

		//����ǰЭ�̷��붨ʱ�����л�����̨���ȵ���ʱ�������Ѻ����л�����
		Fiber::YieldToHold();
		return 0;
	}
	int nanosleep(const struct timespec* req, struct timespec* rem)
	{
		//���û��hookס����ֱ�ӵ���ԭʼ�Ŀⷽ��
		if (!t_hook_enable)
		{
			return nanosleep_origin(req, rem);
		}

		int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
		//��ȡ��ǰЭ�̺͵�ǰIO������
		shared_ptr<Fiber> fiber = Fiber::GetThis();
		IOManager* iomanager = IOManager::GetThis();

		//bind()���յõ����Ǽ̳���Scheduler���IOManager::schedule
		shared_ptr<Timer> timer(new Timer(timeout_ms, bind((void(Scheduler::*)
			(shared_ptr<Fiber>, int thread)) & IOManager::schedule, iomanager, fiber, -1)));
		//shared_ptr<Timer> timer(new Timer(timeout_ms, [iomanager, fiber]() {iomanager->schedule(fiber); }));
		iomanager->addTimer(timer);

		//����ǰЭ�̷��붨ʱ�����л�����̨���ȵ���ʱ�������Ѻ����л�����
		Fiber::YieldToHold();
		return 0;
	}


	//socket���
	int socket(int domain, int type, int protocol)
	{
		//���û��hookס����ֱ�ӵ���ԭʼ�Ŀⷽ��
		if (!t_hook_enable)
		{
			return socket_origin(domain, type, protocol);
		}

		//����ԭʼ�Ŀⷽ������socket
		int file_descriptor = socket_origin(domain, type, protocol);

		//�������ʧ�ܣ�����-1�����ֺ�ԭʼ�Ŀⷽ����ͬ����Ϊ��
		if (file_descriptor == -1)
		{
			return file_descriptor;
		}
		//���򴴽��ɹ�
		else
		{
			//���õ����ļ������������ߴ�����Ӧ���ļ�������ʵ��
			Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(file_descriptor, true);
			//�����ļ�������
			return file_descriptor;
		}
	}
	//����ָ���ĳ�ʱʱ�䳢������
	int connect_with_timeout(int sockfd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)
	{
		//���û��hookס����ֱ�ӵ���ԭʼ�Ŀⷽ��
		if (!t_hook_enable)
		{
			return connect_origin(sockfd, addr, addrlen);
		}

		//��ȡsocket��Ӧ���ļ�������ʵ��
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(sockfd);
		//����õ����ļ�������ʵ��Ϊ�ջ��ڹر�״̬
		if (!file_descriptor_entity || file_descriptor_entity->isClose())
		{
			//���ô�����Ϊ��bad file descriptor��
			errno = EBADF;
			//����-1�����ֺ�ԭʼ�Ŀⷽ����ͬ����Ϊ��
			return -1;
		}
		//����õ����ļ�������ʵ�岻��socket���û���������Ϊ��������ֱ�ӵ���ԭʼ�Ŀⷽ��
		if (!file_descriptor_entity->isSocket() || file_descriptor_entity->isUserNonblock())
		{
			return connect_origin(sockfd, addr, addrlen);
		}

		//����ԭʼ�Ŀⷽ����������
		int return_value = connect_origin(sockfd, addr, addrlen);
		//�������0˵�����ӳɹ���ֱ�ӷ���0
		if (return_value == 0)
		{
			return 0;
		}
		//�������ֵ����-1�������̫���ܣ������˳�����δ�����������Ĵ���ֱ�ӷ���ԭʼ�ⷽ���ķ���ֵ
		else if (return_value != -1 || errno != EINPROGRESS)
		{
			return return_value;
		}
		//����˵������δ������ɣ������첽����
		else
		{
			//��ȡ��ǰIO������
			IOManager* iomanager = IOManager::GetThis();

			shared_ptr<Timer> timer;
			//��־��ʱ���Ƿ�ȡ����shared_ptr
			shared_ptr<int> timer_cancelled(new int);
			//��timer_cancelled��ʼ��Ϊ0����ʾδȡ��
			*timer_cancelled = 0;
			//��־��ʱ���Ƿ�ȡ����weak_ptr������ʱ����ʱ������
			weak_ptr<int> timer_cancelled_weak(timer_cancelled);

			//��������˳�ʱʱ�䣨��ʱʱ�䲻Ϊ-1��
			if (timeout_ms != (uint64_t)-1)
			{
				//���ö�ʱ���������ʱ��ʱ����δʧЧ��������¼�
				timer.reset(new Timer(timeout_ms, [timer_cancelled_weak, sockfd, iomanager]()
					{
						//����������weak_ptr
						auto condition = timer_cancelled_weak.lock();
						//��������Ѿ�ʧЧ��connect_with_timeout����epollϵͳ���������أ���ֱ�ӷ���
						if (!condition)
						{
							return;
						}
						//�������ô����벢����Э���¼�������connect_with_timeout�ĵ�ǰЭ��
						else
						{
							//���ò�����ʱ�����룬��ʾ��socket��д֮ǰ�¼��ѳ�ʱ
							*condition = ETIMEDOUT;
							//�����socket��д���¼�����Ϊ��connect_with_timeout�ĵ�ǰЭ�̻��ѣ������ģ�
							iomanager->settleEvent(sockfd, IOManager::WRITE);
						}
					}
				, timer_cancelled_weak));
				//�������úõĶ�ʱ������ʱ����������
				iomanager->addTimer(timer);
			}

			//���Խ���ǰЭ����Ϊд���¼����ӵ�IO�������У���Ϊ�¼����ûص�������Ĭ��ʹ�õ�ǰЭ��Ϊ�ص�������
			bool return_value = iomanager->addEvent(sockfd, IOManager::WRITE, nullptr);
			//������ӳɹ�
			if (return_value == true)
			{
				//����ǰЭ�̣��ȴ���ʱ�����ѻ�epoll����д���¼�����socket�Ѿ���д��ʱ������
				Fiber::YieldToHold();

				//��ǰЭ�̱����Ѻ������ʱ����Ϊ�գ�ȡ��֮
				if (timer)
				{
					iomanager->getTimer_manager()->cancelTimer(timer);
				}
				//�����socket��д֮ǰ�¼��ѳ�ʱ��˵��Э�̱����ѵ�ԭ���Ƕ�ʱ����ʱ�����ô����벢����-1
				if (*timer_cancelled!=0)
				{
					errno = *timer_cancelled;
					return -1;
				}
			}
			//��������ʧ�ܣ�ȡ����ʱ��������
			else
			{
				//�����ʱ����Ϊ�գ�ȡ��֮
				if (timer)
				{
					iomanager->getTimer_manager()->cancelTimer(timer);
				}

				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "connect addEvent(" << sockfd << ", WRITE)";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			}



			//��ѯsocket���ӽ��
			int error = 0;
			socklen_t len = sizeof(int);
			//�������ʧ���򷵻�-1
			if (-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len))
			{
				return -1;
			}
			//������ӳɹ���û�г������򷵻�0
			else if (!error)
			{
				return 0;
			}
			//������ӳɹ������������ô����벢����-1
			else
			{
				errno = error;
				return -1;
			}
		}
	}
	//����Ĭ�ϵ�tcp���ӳ�ʱʱ�䳢������
	int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
	{
		//����ԭʼ���Ƶ��õ�connect��������Ĭ�ϵ�TCP���ӳ�ʱʱ��
		return connect_with_timeout(sockfd, addr, addrlen, s_tcp_connect_timeout);
	}
	int accept(int s, struct sockaddr* addr, socklen_t* addrlen)
	{
		int file_descriptor = do_io(s, accept_origin, "accept", IOManager::READ, SO_RCVTIMEO, addr, addrlen);
		//���ԭʼ�⺯��accept()ִ�гɹ�
		if (file_descriptor >= 0)
		{
			//���õ����ļ������������ߴ�����Ӧ���ļ�������ʵ��
			Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(file_descriptor, true);
		}
		return file_descriptor;
	}

	//read���
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

	//write���
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

	//fd���
	int close(int fd)
	{
		//���û��hookס����ֱ�ӵ���ԭʼ�Ŀⷽ��
		if (!t_hook_enable)
		{
			return close_origin(fd);
		}
		
		//��ȡsocket��Ӧ���ļ�������ʵ��
		shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(fd);
		//���ʵ�岻Ϊ�գ�����socket����������¼���ɾ����ʵ��
		if (file_descriptor_entity)
		{
			auto iomanager = IOManager::GetThis();
			if (iomanager)
			{
				//����socket����������¼�
				iomanager->settleAllEvents(fd);
			}
			//ɾ��socket��Ӧ���ļ�������ʵ��
			Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->deleteFile_descriptor(fd);
		}
		//����ԭʼ�Ŀⷽ���ر�socket
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

		//���request��FIONBIO
		if (FIONBIO == request)
		{
			bool user_nonblock = !!*(int*)arg;
			//��ȡsocket��Ӧ���ļ�������ʵ��
			shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(d);
			//���ʵ��Ϊ�գ���ʵ���ѹرգ���ʵ�岻��socket��ֱ�ӵ���ԭʼ�Ŀⷽ��
			if (!file_descriptor_entity || file_descriptor_entity->isClose() || !file_descriptor_entity->isSocket())
			{
				return ioctl_origin(d, request, arg);
			}
			//�����û��������÷�������ֵ��Ϊarg
			file_descriptor_entity->setUserNonblock(user_nonblock);
		}
		//����ԭʼ�Ŀⷽ��
		return ioctl_origin(d, request, arg);
	}
	int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen)
	{
		//ֱ�ӵ���ԭʼ�Ŀⷽ��
		return getsockopt_origin(sockfd, level, optname, optval, optlen);
	}
	int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen)
	{
		//���û��hookס����ֱ�ӵ���ԭʼ�Ŀⷽ��
		if (!t_hook_enable)
		{
			return setsockopt_origin(sockfd, level, optname, optval, optlen);
		}

		//���Ϊͨ��socket�����ҿ�ѡ��Ϊ���ճ�ʱ���ͳ�ʱ����ȡsocket��Ӧ���ļ�������ʵ�岢���ó�ʱʱ��
		if (level == SOL_SOCKET && (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO))
		{
			//��ȡsocket��Ӧ���ļ�������ʵ��
			shared_ptr<FileDescriptorEntity> file_descriptor_entity = Singleton<FileDescriptorManager>::GetInstance_normal_ptr()->getFile_descriptor(sockfd);
			//����������ļ�������ʵ�岻Ϊ�գ�Ϊ�����ó�ʱʱ��
			if (file_descriptor_entity)
			{
				const timeval* tv = (const timeval*)optval;
				file_descriptor_entity->setTimeout(optname, tv->tv_sec * 1000 + tv->tv_usec / 1000);
			}
		}
		//����ԭʼ�Ŀⷽ��
		return setsockopt_origin(sockfd, level, optname, optval, optlen);
	}
}
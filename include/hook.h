#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

/*
* hookϵͳ���ܣ�
* 1.hookϵͳ��д�˶��ԭʼ�⺯����ʹ������ϵͳ�ڰ�ԭ���������ö�Ӧ����ʱ��Ϊ����hook�汾
* 2.hook�汾�ĺ���������ʱƾ��epollϵͳ�Ͷ�ʱ��ϵͳ�Բ����첽���������������
*/

namespace HookSpace
{
	//tcp���ӳ�ʱʱ��
	extern uint64_t s_tcp_connect_timeout;
	//�߳�ר����̬��������ʾ��ǰ�߳��Ƿ�hookס���Ƿ�����hook��
	extern thread_local bool t_hook_enable;
}

//����C������,����ԭʼ�⺯��
extern "C"
{
	//sleep���
	extern unsigned int (*sleep_origin)(unsigned int);
	extern int (*usleep_origin)(useconds_t usec);
	extern int (*nanosleep_origin)(const struct timespec* req, struct timespec* rem);

	//socket���
	extern int (*socket_origin)(int domain, int type, int protocol);
	extern int (*connect_origin)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
	extern int (*accept_origin)(int s, struct sockaddr* addr, socklen_t* addrlen);

	//read���
	extern ssize_t(*read_origin)(int fd, void* buf, size_t count);
	extern ssize_t(*readv_origin)(int fd, const struct iovec* iov, int iovcnt);
	extern ssize_t(*recv_origin)(int sockfd, void* buf, size_t len, int flags);
	extern ssize_t(*recvfrom_origin)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
	extern ssize_t(*recvmsg_origin)(int sockfd, struct msghdr* msg, int flags);

	//write���
	extern ssize_t(*write_origin)(int fd, const void* buf, size_t count);
	extern ssize_t(*writev_origin)(int fd, const struct iovec* iov, int iovcnt);
	extern ssize_t(*send_origin)(int s, const void* msg, size_t len, int flags);
	extern ssize_t(*sendto_origin)(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen);
	extern ssize_t(*sendmsg_origin)(int s, const struct msghdr* msg, int flags);

	//fd���
	extern int (*close_origin)(int fd);
	extern int (*fcntl_origin)(int fd, int cmd, .../* arg */);
	extern int (*ioctl_origin)(int d, unsigned long int request, .../* arg */);
	extern int (*getsockopt_origin)(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
	extern int (*setsockopt_origin)(int sockfd, int level, int optname, const void* optval, socklen_t optlen);

	extern int connect_with_timeout(int sockfd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);
}
#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>


namespace HookSpace
{
	void set_hook_enable(const bool flag);
}

//按照C风格编译
extern "C"
{
	//sleep相关
	extern unsigned int (*sleep_f)(unsigned int);
	extern int (*usleep_f)(useconds_t usec);
	extern int (*nanosleep_f)(const struct timespec* req, struct timespec* rem);

	//socket相关
	extern int (*socket_f)(int domain, int type, int protocol);
	extern int (*connect_f)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
	extern int (*accept_f)(int s, struct sockaddr* addr, socklen_t* addrlen);

	//read相关
	extern ssize_t (*read_f)(int fd, void* buf,size_t count);
	extern ssize_t(*readv_f)(int fd, const struct iovec*iov, int iovcnt);
	extern ssize_t(*recv_f)(int sockfd, void* buf, size_t len, int flags);
	extern ssize_t(*recvfrom_f)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
	extern ssize_t(*recvmsg_f)(int sockfd, struct msghdr* msg, int flags);

	//write相关
	extern ssize_t(*write_f)(int fd,const void* buf, size_t count);
	extern ssize_t(*writev_f)(int fd, const struct iovec* iov, int iovcnt);
	extern ssize_t(*send_f)(int s,const void* msg, size_t len, int flags);
	extern ssize_t(*sendto_f)(int s,const void* msg, size_t len, int flags,const struct sockaddr* to, socklen_t tolen);
	extern ssize_t(*sendmsg_f)(int s,const struct msghdr* msg, int flags);

	extern int (*close_f)(int fd);
	extern int (*fcntl_f)(int fd, int cmd, .../* arg */);
	extern int (*ioctl_f)(int d, unsigned long int request, .../* arg */);

	extern int (*getsockopt_f)(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
	extern int (*setsockopt_f)(int sockfd, int level, int optname,const void* optval, socklen_t optlen);
}


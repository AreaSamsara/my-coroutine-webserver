#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>


namespace HookSpace
{
	//tcp连接超时时间
	extern uint64_t s_tcp_connect_timeout;
	//线程专属静态变量，表示当前线程是否hook住（是否启用hook）
	extern thread_local bool t_hook_enable;
}

//按照C风格编译,声明原始库函数
extern "C"
{
	//sleep相关
	extern unsigned int (*sleep_origin)(unsigned int);
	extern int (*usleep_origin)(useconds_t usec);
	extern int (*nanosleep_origin)(const struct timespec* req, struct timespec* rem);

	//socket相关
	extern int (*socket_origin)(int domain, int type, int protocol);
	extern int (*connect_origin)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
	extern int (*accept_origin)(int s, struct sockaddr* addr, socklen_t* addrlen);

	//read相关
	extern ssize_t(*read_origin)(int fd, void* buf, size_t count);
	extern ssize_t(*readv_origin)(int fd, const struct iovec* iov, int iovcnt);
	extern ssize_t(*recv_origin)(int sockfd, void* buf, size_t len, int flags);
	extern ssize_t(*recvfrom_origin)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen);
	extern ssize_t(*recvmsg_origin)(int sockfd, struct msghdr* msg, int flags);

	//write相关
	extern ssize_t(*write_origin)(int fd, const void* buf, size_t count);
	extern ssize_t(*writev_origin)(int fd, const struct iovec* iov, int iovcnt);
	extern ssize_t(*send_origin)(int s, const void* msg, size_t len, int flags);
	extern ssize_t(*sendto_origin)(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen);
	extern ssize_t(*sendmsg_origin)(int s, const struct msghdr* msg, int flags);

	//fd相关
	extern int (*close_origin)(int fd);
	extern int (*fcntl_origin)(int fd, int cmd, .../* arg */);
	extern int (*ioctl_origin)(int d, unsigned long int request, .../* arg */);
	extern int (*getsockopt_origin)(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
	extern int (*setsockopt_origin)(int sockfd, int level, int optname, const void* optval, socklen_t optlen);

	extern int connect_with_timeout(int sockfd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);
}
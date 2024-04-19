#include "utility.h"

namespace UtilitySpace
{
	//获取当前线程id
	pid_t getThread_id()
	{
		//用流获取线程在进程内的id
		/*stringstream ss;
		ss << std::this_thread::get_id();
		pid_t thread_id = atoi(ss.str().c_str());
		return thread_id;*/

		//调用系统函数获取线程在系统中的id
		return syscall(SYS_gettid);
	}
}
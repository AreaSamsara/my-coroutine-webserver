#include "utility.h"

namespace UtilitySpace
{
	//��ȡ��ǰ�߳�id
	pid_t getThread_id()
	{
		//������ȡ�߳��ڽ����ڵ�id
		/*stringstream ss;
		ss << std::this_thread::get_id();
		pid_t thread_id = atoi(ss.str().c_str());
		return thread_id;*/

		//����ϵͳ������ȡ�߳���ϵͳ�е�id
		return syscall(SYS_gettid);
	}
}
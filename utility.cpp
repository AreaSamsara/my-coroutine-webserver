#include "utility.h"

namespace UtilitySpace
{
	//��ȡ��ǰ�߳�id
	uint32_t getThread_id()
	{
		//������ȡ�߳�id
		stringstream ss;
		ss << std::this_thread::get_id();
		uint32_t thread_id = atoi(ss.str().c_str());
		return thread_id;
	}
}
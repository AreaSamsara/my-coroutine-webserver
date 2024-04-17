#include "utility.h"

namespace UtilitySpace
{
	//获取当前线程id
	uint32_t getThread_id()
	{
		//用流获取线程id
		stringstream ss;
		ss << std::this_thread::get_id();
		uint32_t thread_id = atoi(ss.str().c_str());
		return thread_id;
	}
}
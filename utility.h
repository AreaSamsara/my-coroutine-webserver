#pragma once
#include <iostream>
#include <sstream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>

namespace UtilitySpace
{
	using std::stringstream;

	//��ȡ��ǰ�߳�id
	pid_t getThread_id();
}
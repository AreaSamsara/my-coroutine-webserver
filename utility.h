#pragma once
#include <iostream>
#include <sstream>
#include <cstring>
#include <thread>

namespace UtilitySpace
{
	using std::stringstream;

	//获取当前线程id
	uint32_t getThread_id();
}
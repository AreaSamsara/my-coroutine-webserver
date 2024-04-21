#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>
#include <execinfo.h>

namespace UtilitySpace
{
	using std::stringstream;
	using std::string;
	using std::vector;

	//获取当前线程id
	pid_t GetThread_id();

	//获取当前协程id
	//pid_t GetThread_id();

	void Backtrace(vector<string>& bt, const int size, const int skip = 1);

	string BacktraceToString(const int size, const int skip = 2, const string& prefix = "");
}
#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <execinfo.h>

#include "log.h"

namespace UtilitySpace
{
	using namespace LogSpace;
	using std::stringstream;
	using std::string;
	using std::vector;
	using std::shared_ptr;
	

	//获取当前线程id
	pid_t GetThread_id();

	//获取当前线程名称
	string GetThread_name();

	//获取当前协程id
	uint32_t GetFiber_id();

	void Backtrace(vector<string>& bt, const int size, const int skip = 1);

	string BacktraceToString(const int size, const int skip = 2, const string& prefix = "");

	void Assert(shared_ptr<LogEvent> event);

	void Assert(shared_ptr<LogEvent> event, const string& message);
}
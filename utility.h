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
	

	//��ȡ��ǰ�߳�id
	pid_t GetThread_id();

	//��ȡ��ǰ�߳�����
	string GetThread_name();

	//��ȡ��ǰЭ��id
	uint32_t GetFiber_id();

	void Backtrace(vector<string>& bt, const int size, const int skip = 1);

	string BacktraceToString(const int size, const int skip = 2, const string& prefix = "");

	void Assert(shared_ptr<LogEvent> event);

	void Assert(shared_ptr<LogEvent> event, const string& message);
}
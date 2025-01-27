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
	
#if defined __GNUC__ || defined __llvm__
	/// LIKCLY ��ķ�װ, ���߱������Ż�,��������ʳ���
#   define SYLAR_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY ��ķ�װ, ���߱������Ż�,��������ʲ�����
#   define SYLAR_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define SYLAR_LIKELY(x)      (x)
#   define SYLAR_UNLIKELY(x)      (x)
#endif

	//��ȡ��ǰ�߳�id
	pid_t GetThread_id();
	//��ȡ��ǰ�߳�����
	string GetThread_name();

	//��ȡ��ǰЭ��id
	uint32_t GetFiber_id();

	//ʱ��ms
	uint64_t GetCurrentMS();
	//ʱ��us
	uint64_t GetCurrentUS();

	void Backtrace(vector<string>& bt, const int size, const int skip = 1);

	string BacktraceToString(const int size = 64, const int skip = 2, const string& prefix = "");

	//����ջ������ջ��Ϣ�����жϳ���
	void Assert(shared_ptr<LogEvent> event);

	//����ջ������ջ��Ϣ��message�ַ��������жϳ���
	void Assert(shared_ptr<LogEvent> event, const string& message);
}
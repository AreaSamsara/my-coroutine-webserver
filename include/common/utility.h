#pragma once
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <execinfo.h>

#include "common/log.h"

namespace UtilitySpace
{
	using namespace LogSpace;
	using std::shared_ptr;
	using std::string;
	using std::stringstream;
	using std::vector;

#if defined __GNUC__ || defined __llvm__
	// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#define SYLAR_LIKELY(x) __builtin_expect(!!(x), 1)
	// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define SYLAR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define SYLAR_LIKELY(x) (x)
#define SYLAR_UNLIKELY(x) (x)
#endif

	// 获取当前线程id
	pid_t GetThread_id();
	// 获取当前线程名称
	string GetThread_name();

	// 获取当前协程id
	uint32_t GetFiber_id();

	// 获取当前时间，单位为ms
	uint64_t GetCurrentMS();
	// 获取当前时间，单位为us
	uint64_t GetCurrentUS();

	void Backtrace(vector<string> &bt, const int size, const int skip = 1);

	string BacktraceToString(const int size = 64, const int skip = 2, const string &prefix = "");

	// 回溯栈并发送栈消息，再中断程序
	void Assert(shared_ptr<LogEvent> event);

	// 回溯栈并发送栈消息和message字符串，再中断程序
	void Assert(shared_ptr<LogEvent> event, const string &message);
}
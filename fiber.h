#pragma once
#include <ucontext.h>
#include "thread.h"

namespace FiberSpace
{
	using std::enable_shared_from_this;
	using std::shared_ptr;
	using std::function;

	class Fiber : public enable_shared_from_this<Fiber>
	{
	public:
		enum State
		{
			INIT,
			HOLD,
			EXEC,
			TERM,
			READY,
			EXCEPT
		};
	private:
		Fiber();
	public:
		Fiber(function<void()> callback, size_t stacksize = 1024 * 1024);
		~Fiber();

		//重置协程函数和状态
		//INIT，TERM
		void reset(function<void()> callback);
		//切换到当前协程执行
		void swapIn();
		//切换到后台执行
		void swapOut();

		//获取协程id
		uint64_t getId()const { return m_id; }
	public:
		//设置当前协程
		static void SetThis(Fiber* fiber);
		//获取当前协程
		static shared_ptr<Fiber> GetThis();
		//协程切换到后台并设置为Ready状态
		static void YieldTOReady();
		//协程切换到后台并设置为Hold状态
		static void YieldTOHold();
		//总协程数
		static uint64_t TotalFibers();

		static void MainFunc();
		//获取当前协程id
		static uint64_t GetFiber_id();

	private:
		uint64_t m_id = 0;
		uint32_t m_stacksize = 0;
		State m_state = INIT;

		ucontext_t m_context;
		void* m_stack = nullptr;

		function<void()> m_callback;
	};


	class MallocStackAllocator
	{
	public:
		static void* Allocate(size_t size)
		{
			return malloc(size);
		}

		static void Deallocate(void* vp, size_t size)
		{
			return free(vp);
		}
	};
}
#pragma once
#include <ucontext.h>
#include <atomic>

#include "thread.h"

namespace FiberSpace
{
	using std::enable_shared_from_this;
	using std::shared_ptr;
	using std::function;
	using std::atomic;

	class Fiber : public enable_shared_from_this<Fiber>
	{
	public:
		//表示协程状态的枚举类型
		enum State
		{
			//初始化状态
			INITIALIZE,
			//挂起状态
			HOLD,
			//执行状态
			EXECUTE,
			//终止状态
			TERMINAL,
			//准备状态
			READY,
			//异常状态
			EXCEPT
		};
	private:
		//默认构造函数，私有方法，仅在初次创建主协程时被静态的GetThis()方法调用
		Fiber();
	public:
		//用于创建子协程的构造函数
		Fiber(function<void()> callback, size_t stacksize = 1024 * 1024);
		~Fiber();

		//重置协程函数和状态，用于免去内存的再分配，在空闲的已分配内存上直接创建新协程
		void reset(function<void()> callback);
		//切换到本协程执行
		void swapIn();
		//将本协程切换到后台
		void swapOut();

		//void call();
		//void back();

		//获取协程id
		uint64_t getId()const { return m_id; }

		//获取协程状态
		State getState()const { return m_state; }
		//设置协程状态
		void setState(State state) { m_state = state; }
	public:
		////设置当前协程
		//static void SetThis(Fiber* fiber);
		//获取当前协程，并仅在第一次调用时创建主协程
		static shared_ptr<Fiber> GetThis();

		//将协程切换到后台并设置为Ready状态
		static void YieldTOReady();
		//将协程切换到后台并设置为Hold状态
		static void YieldTOHold();

		////获取总协程数
		//static uint64_t GetFiber_count();
		//获取当前协程id
		static uint64_t GetFiber_id();

		//协程的主运行函数
		static void MainFunction();
		//static void CallerMainFunction();
	private:
		//协程id
		uint64_t m_id = 0;
		//协程状态
		State m_state = INITIALIZE;
		//协程栈大小，默认为0（因为主协程的栈大小为0）
		uint32_t m_stacksize = 0;
		//协程栈
		void* m_stack = nullptr;
		//协程语境
		ucontext_t m_context;
		//协程回调函数
		function<void()> m_callback;
	private:
		//下一个新协程的id（静态原子类型，保证读写的线程安全）
		static atomic<uint64_t> s_new_fiber_id;
		//协程总数（静态原子类型，保证读写的线程安全）
		static atomic<uint64_t> s_fiber_count;
	public:
		//每个线程专属的当前协程（线程专属变量的生命周期由线程自主管理，故使用裸指针）
		static thread_local Fiber* t_fiber;
		//每个线程专属的主协程
		static thread_local shared_ptr <Fiber> t_main_fiber;
	};

	//栈内存分配类
	class MallocStackAllocator
	{
	public:
		static void* Allocate(const size_t size)
		{
			return malloc(size);
		}

		static void Deallocate(void* memory, const size_t size)
		{
			return free(memory);
		}
	};
}
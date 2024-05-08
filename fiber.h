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

	/*
	* 协程类调用方法（一般由协程调度器直接调用）：
	* 1.先在线程内首次调用Fiber::GetThis()静态方法以创建栈为空的主协程，作为操控子协程的媒介
	* 2.再在线程内调用构造函数创建子协程，为其分配栈和回调函数，并使用swapIn()方法切换到该协程执行
	* 3.协程的回调函数执行完毕以后会自动使用swapOut()方法切换回当前线程的主协程
	* 4.*如果需要实现协程的灵活切换，可以在回调函数内部使用swapOut()、YieldToReady()、YieldToHold()等方法
	*/

	//协程类（公有继承自enable_shared_from_this类，以使用shared_from_this()方法）
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
	public:
		//用于创建子协程的构造函数
		Fiber(const function<void()>& callback, const size_t stacksize = 1024 * 1024);
		~Fiber();

		//重置协程函数和状态，用于免去内存的再分配，在空闲的已分配内存上直接创建新协程
		void reset(const function<void()>& callback);
		//切换到本协程执行
		void swapIn();
		//将本协程切换到后台
		void swapOut();

		//获取协程id
		uint64_t getId()const { return m_id; }

		//获取协程状态
		State getState()const { return m_state; }
		//设置协程状态
		void setState(const State state) { m_state = state; }
	public:
		//获取当前协程，并仅在第一次调用时创建主协程
		static shared_ptr<Fiber> GetThis();

		//将协程切换到后台并设置为Ready状态
		static void YieldToReady();
		//将协程切换到后台并设置为Hold状态
		static void YieldToHold();

		//获取当前协程id
		static uint64_t GetFiber_id();

		//协程的主运行函数
		static void MainFunction();
	private:
		//默认构造函数，私有方法，仅在初次创建主协程时被静态的GetThis()方法调用
		Fiber();
	public:
		//每个线程专属的当前协程（线程专属变量的生命周期由线程自主管理，故使用裸指针）
		static thread_local Fiber* t_fiber;
		//每个线程专属的主协程
		static thread_local shared_ptr <Fiber> t_main_fiber;
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
	};

	////栈内存分配类
	//class MallocStackAllocator
	//{
	//public:
	//	static void* Allocate(const size_t size)
	//	{
	//		return malloc(size);
	//	}

	//	static void Deallocate(void* memory, const size_t size)
	//	{
	//		return free(memory);
	//	}
	//};
}
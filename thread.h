#pragma once
#include <functional>

#include "singleton.h"
#include "utility.h"
#include "log.h"

namespace ThreadSpace
{
	using namespace SingletonSpace;
	using namespace LogSpace;
	using namespace UtilitySpace;
	using std::function;

	/*
	* 线程类调用方法：
	* 1.先用想执行的函数对象作为回调函数创建Thread对象，即可开始线程的执行
	* 2.调用Thread对象的join()方法，可以阻塞主线程，让主线程等待该线程执行完毕再继续
	*/

	//线程类
	class Thread
	{
	public:
		Thread(const function<void()>& callback, const string& name);
		//析构Thread对象并将线程设置为分离状态
		~Thread();

		//获取私有成员
		pid_t getId()const { return m_id; }
		const string& getName()const { return m_name; }

		//阻塞正在运行的进程，将thread加入等待队列
		void join();
	public:
		//获取线程专属的Thread类指针，设置为静态方法以访问静态类型
		static Thread* getThis();
	private:
		//删除复制构造函数
		Thread(const Thread&) = delete;
		//删除移动构造函数
		Thread(const Thread&&) = delete;
		//删除赋值运算符
		Thread& operator=(const Thread&) = delete;
	private:
		//传递给pthread_create的主运行函数
		static void* run(void* arg);
	public:
		static thread_local Thread* t_thread;	//线程专属的Thread类指针（线程专属变量的生命周期由线程自主管理，故使用裸指针）
	private:
		pthread_t m_thread = 0;		//线程（默认设为0表示尚未创建）
		string m_name;				//线程名称
		pid_t m_id = -1;			//线程id
		function<void()> m_callback;	//线程回调函数
		
		Semaphore m_semaphore;		//信号量
	};
}

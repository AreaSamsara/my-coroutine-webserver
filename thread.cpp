#pragma once
#include "thread.h"

namespace ThreadSpace
{
	//class Thread
	Thread::Thread(function<void()> callback, const string& name) :m_callback(callback), m_name(name)
	{
		//如果输入的名称为空字符串，改为"UNKNOW"
		if (name.empty())
		{
			m_name = "UNKNOW";
		}

		//创建线程
		//int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
		int return_value = pthread_create(&m_thread, nullptr, &run, this);
		if (return_value != 0)		//返回0表示创建成功，否则报错
		{
			//设置日志事件
			//__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
			event->getSstream() << "pthread_create thread fail, return_value=" << return_value << " name=" << name;
			//使用LoggerManager单例的默认logger输出日志
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);

			throw logic_error("pthread_create error");
		}

		//阻塞主线程，等待信号量大于0（run函数的指令）再结束构造函数
		m_semaphore.wait();
	}

	//析构Thread对象并将线程设置为分离状态
	Thread::~Thread()
	{
		//如果线程存在，将线程设置为分离状态
		if (m_thread)
		{
			pthread_detach(m_thread);
		}
	}

	//阻塞正在运行的进程，将thread加入进程队列并开始执行
	void Thread::join()
	{
		//如果线程存在，将线程加入线程队列
		if (m_thread)
		{
			int return_value = pthread_join(m_thread,nullptr);
			if (return_value != 0)		//成功加入进程队列则返回0，否则报错
			{
				//设置日志事件
				//__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
				event->getSstream() << "pthread_join thread fail, return_value=" << return_value << " name=" << m_name;
				throw logic_error("pthread_join error");
				//使用LoggerManager单例的默认logger输出日志
				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			}

			//线程已加入队列并执行，重置m_thread为0
			m_thread = 0;
		}
	}

	//传递给pthread_create的主运行函数
	void* Thread::run(void* arg)
	{
		//接受Thread*类型参数（当前线程对象）
		Thread* thread = (Thread*)arg;

		//设置静态变量t_thread
		t_thread = thread;
		//t_thread_name = thread->m_name;

		//获取线程id
		thread->m_id = UtilitySpace::GetThread_id();
		//设置线程名称（被限制在15个字符以内）
		pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
		
		//将thread的回调函数复制到局部变量再运行，确保thread的自由
		function<void()> callback;
		callback.swap(thread->m_callback);

		//基本设置已经结束，先增加信号量解放主线程，再运行回调函数
		thread->m_semaphore.notify();
		callback();

		return 0;
	}

	//获取线程专属的Thread类指针，设置为静态方法以访问静态类型
	Thread* Thread::getThis()
	{
		return t_thread;
	}
	/*const string& Thread::s_getName()
	{
		return t_thread_name;
	}*/
	/*void Thread::setName(const string& name)
	{
		if (t_thread)
		{
			t_thread->m_name = name;
		}
		t_thread_name = name;
	}*/

	thread_local Thread* Thread::t_thread = nullptr;	//线程专属的Thread类指针（线程专属变量的生命周期由线程自主管理，故使用裸指针）
	//static thread_local string t_thread_name = "UNKNOW";
}

#pragma once
#include "thread.h"

namespace ThreadSpace
{
	//class Semaphore
	Semaphore::Semaphore(uint32_t count)
	{
		if (sem_init(&m_semaphore, 0, count))
		{
			throw logic_error("sem_init error");
		}
	}
	Semaphore::~Semaphore()
	{
		sem_destroy(&m_semaphore);
	}

	void Semaphore::wait()
	{
		if (sem_wait(&m_semaphore))
		{
			throw logic_error("sem_wait error");
		}
	}
	void Semaphore::notify()
	{
		if (sem_post(&m_semaphore))
		{
			throw logic_error("sem_post error");
		}
	}

	//class Thread
	static thread_local Thread* t_thread = nullptr;
	static thread_local string t_thread_name = "UNKNOW";
	Thread* Thread::getThis()
	{
		return t_thread;
	}
	const string& Thread::s_getName()
	{
		return t_thread_name;
	}
	void Thread::setName(const string& name)
	{
		if (t_thread)
		{
			t_thread->m_name = name;
		}
		t_thread_name = name;
	}
	Thread::Thread(function<void()> cb, const string& name):m_cb(cb),m_name(name)
	{
		if (name.empty())
		{
			m_name = "UNKNOW";
		}
		int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
		if (rt)
		{
			//设置日志事件
			//__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
			event->getSstream() << "pthread_create thread fail, rt=" << rt << " name=" << name;
			throw logic_error("pthread_create error");
			//使用LoggerManager单例的默认logger输出日志
			Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
		}
		m_semaphore.wait();
	}

	Thread::~Thread()
	{
		if (m_thread)
		{
			pthread_detach(m_thread);
		}
	}

	void Thread::join()
	{
		if (m_thread)
		{
			int rt = pthread_join(m_thread,nullptr);
			if (rt)
			{
				//设置日志事件
				//__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
				event->getSstream() << "pthread_join thread fail, rt=" << rt << " name=" << m_name;
				throw logic_error("pthread_join error");
				//使用LoggerManager单例的默认logger输出日志
				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			}
			m_thread = 0;
		}
	}

	void* Thread::run(void* arg)
	{
		Thread* thread = (Thread*)arg;
		t_thread = thread;
		t_thread_name = thread->m_name;
		thread->m_id = UtilitySpace::getThread_id();
		pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
		
		function<void()> cb;
		cb.swap(thread->m_cb);

		thread->m_semaphore.notify();

		cb();
		return 0;
	}

	
}

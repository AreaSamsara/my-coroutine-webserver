#pragma once
#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>

#include "log.h"
#include "singleton.h"
#include "utility.h"

namespace ThreadSpace
{
	using namespace LogSpace;
	using namespace SingletonSpace;
	using std::string;
	using std::function;
	using std::logic_error;

	class Semaphore
	{
	public:
		Semaphore(uint32_t count=0);
		~Semaphore();

		void wait();
		void notify();
	private:
		Semaphore(const Semaphore&) = delete;
		Semaphore(const Semaphore&&) = delete;
		Semaphore& operator=(const Semaphore&) = delete;
	private:
		sem_t m_semaphore;
	};



	template<class T>
	struct ScopedLockImpl
	{
	public:
		ScopedLockImpl(T& mutex) :m_mutex(mutex)
		{
			m_mutex.lock();
			m_locked = true;
		}
		~ScopedLockImpl()
		{
			unlock();
		}

		void lock()
		{
			if (!m_locked)
			{
				m_mutex.lock();
				m_locked = true;
			}
		}
		void unlock()
		{
			if (m_locked)
			{
				m_mutex.unlock();
				m_locked = false;
			}
		}
	private:
		T& m_mutex;
		bool m_locked;
	};



	template<class T>
	struct ReadScopedLockImpl
	{
	public:
		ReadScopedLockImpl(T& mutex) :m_mutex(mutex)
		{
			m_mutex.read_lock();
			m_locked = true;
		}
		~ReadScopedLockImpl()
		{
			unlock();
		}

		void lock()
		{
			if (!m_locked)
			{
				m_mutex.read_lock();
				m_locked = true;
			}
		}
		void unlock()
		{
			if (m_locked)
			{
				m_mutex.unlock();
				m_locked = false;
			}
		}
	private:
		T& m_mutex;
		bool m_locked;
	};



	template<class T>
	struct WriteScopedLockImpl
	{
	public:
		WriteScopedLockImpl(T& mutex) :m_mutex(mutex)
		{
			m_mutex.write_lock();
			m_locked = true;
		}
		~WriteScopedLockImpl()
		{
			unlock();
		}

		void lock()
		{
			if (!m_locked)
			{
				m_mutex.write_lock();
				m_locked = true;
			}
		}
		void unlock()
		{
			if (m_locked)
			{
				m_mutex.unlock();
				m_locked = false;
			}
		}
	private:
		T& m_mutex;
		bool m_locked;
	};



	class RWMutex
	{
	public:
		RWMutex()
		{
			pthread_rwlock_init(&m_lock, nullptr);
		}
		~RWMutex()
		{
			pthread_rwlock_destroy(&m_lock);
		}

		void read_lock()
		{
			pthread_rwlock_rdlock(&m_lock);
		}
		void write_lock()
		{
			pthread_rwlock_wrlock(&m_lock);
		}
		void unlock()
		{
			pthread_rwlock_unlock(&m_lock);
		}
	private:
		pthread_rwlock_t m_lock;
	};



	class Thread
	{
	public:
		Thread(function<void()> cb, const string& name);
		~Thread();

		pid_t getId()const { return m_id; }
		const string& getName()const { return m_name; }

		void join();

		static Thread* getThis();
		static const string& s_getName();
		static void setName(const string& name);
	private:
		//删除复制构造函数
		Thread(const Thread&) = delete;
		//删除移动构造函数
		Thread(const Thread&&) = delete;
		//删除赋值运算符
		Thread& operator=(const Thread&) = delete;

		static void* run(void* arg);
	private:
		pid_t m_id = -1;
		pthread_t m_thread=0;
		function<void()> m_cb;
		string m_name;

		Semaphore m_semaphore;
	};
}

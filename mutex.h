#pragma once
#include <semaphore.h>
#include <stdint.h>
#include <stdexcept>

namespace MutexSpace
{
	using std::logic_error;

	//被监视的通用互斥锁
	template<class T>
	class ScopedLock
	{
	public:
		//创建ScopedLock对象并将互斥锁锁定
		ScopedLock(T& mutex) :m_mutex(mutex)
		{
			m_mutex.lock();
			m_is_locked = true;
		}

		//析构ScopedLock对象并将互斥锁解锁
		~ScopedLock()
		{
			unlock();
		}

		//锁定
		void lock()
		{
			//如果互斥锁未锁定，则锁定
			if (!m_is_locked)
			{
				m_mutex.lock();
				m_is_locked = true;
			}
		}

		//解锁
		void unlock()
		{
			//如果互斥锁已锁定，则解锁
			if (m_is_locked)
			{
				m_mutex.unlock();
				m_is_locked = false;
			}
		}
	private:
		T& m_mutex;			//互斥锁
		bool m_is_locked;	//锁定状态
	};



	//被监视的读取锁
	template<class T>
	struct ReadScopedLock
	{
	public:
		//创建ReadScopedLock对象并将读取锁锁定
		ReadScopedLock(T& mutex) :m_mutex(mutex)
		{
			m_mutex.read_lock();
			m_locked = true;
		}

		//析构ReadScopedLock对象并将读取锁解锁
		~ReadScopedLock()
		{
			unlock();
		}

		//锁定
		void lock()
		{
			//如果读取锁未锁定，则锁定
			if (!m_locked)
			{
				m_mutex.read_lock();
				m_locked = true;
			}
		}

		//解锁
		void unlock()
		{
			//如果读取锁已锁定，则解锁
			if (m_locked)
			{
				m_mutex.unlock();
				m_locked = false;
			}
		}
	private:
		T& m_mutex;		//读取锁
		bool m_locked;	//锁定状态
	};



	//被监视的写入锁
	template<class T>
	struct WriteScopedLock
	{
	public:
		//创建WriteScopedLock对象并将写入锁锁定
		WriteScopedLock(T& mutex) :m_mutex(mutex)
		{
			m_mutex.write_lock();
			m_locked = true;
		}

		//析构WriteScopedLock对象并将写入锁解锁
		~WriteScopedLock()
		{
			unlock();
		}

		//锁定
		void lock()
		{
			//如果写入锁未锁定，则锁定
			if (!m_locked)
			{
				m_mutex.write_lock();
				m_locked = true;
			}
		}

		//解锁
		void unlock()
		{
			//如果写入锁已锁定，则解锁
			if (m_locked)
			{
				m_mutex.unlock();
				m_locked = false;
			}
		}
	private:
		T& m_mutex;		//写入锁
		bool m_locked;	//锁定状态
	};


	//通用互斥锁
	class Mutex
	{
	public:
		//创建Mutex_Read_Write对象并初始化读/写锁
		Mutex()
		{
			pthread_mutex_init(&m_mutex, nullptr);
		}

		//析构Mutex_Read_Write对象并销毁读/写锁
		~Mutex()
		{
			pthread_mutex_destroy(&m_mutex);
		}

		//锁定互斥锁
		void lock()
		{
			pthread_mutex_lock(&m_mutex);
		}

		//锁定互斥锁
		void unlock()
		{
			pthread_mutex_unlock(&m_mutex);
		}
	private:
		pthread_mutex_t m_mutex;	//互斥锁
	};


	class NullMutex
	{
	public:
		//什么都不做
		NullMutex() {}
		~NullMutex() {}
		//锁定互斥锁
		void lock() {}
		//锁定互斥锁
		void unlock() {}
	};


	//读/写互斥锁
	class Mutex_Read_Write
	{
	public:
		//创建Mutex_Read_Write对象并初始化读/写锁
		Mutex_Read_Write()
		{
			pthread_rwlock_init(&m_lock, nullptr);
		}

		//析构Mutex_Read_Write对象并销毁读/写锁
		~Mutex_Read_Write()
		{
			pthread_rwlock_destroy(&m_lock);
		}

		//锁定读取锁
		void read_lock()
		{
			pthread_rwlock_rdlock(&m_lock);
		}

		//锁定写入锁
		void write_lock()
		{
			pthread_rwlock_wrlock(&m_lock);
		}

		//锁定读/写锁
		void unlock()
		{
			pthread_rwlock_unlock(&m_lock);
		}

	private:
		pthread_rwlock_t m_lock;	//读/写互斥锁
	};



	class NullMutex_Read_Write
	{
	public:
		//什么都不做
		NullMutex_Read_Write() {}
		~NullMutex_Read_Write() {}
		//锁定读取锁
		void read_lock() {}
		//锁定写入锁
		void write_lock() {}
		//锁定读/写锁
		void unlock() {}
	};



	//信号量类
	class Semaphore
	{
	public:
		//创建Semaphore对象并初始化信号量,count为信号量初始值
		Semaphore(uint32_t count = 0);
		//析构Semaphore对象并销毁信号量
		~Semaphore();

		//阻塞线程直到信号量大于0，并将其减一
		void wait();
		//将信号量加一
		void notify();
	private:
		//删除复制构造函数
		Semaphore(const Semaphore&) = delete;
		//删除移动构造函数
		Semaphore(const Semaphore&&) = delete;
		//删除赋值运算符
		Semaphore& operator=(const Semaphore&) = delete;
	private:
		sem_t m_semaphore;		//信号量
	};
}
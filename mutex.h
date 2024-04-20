#pragma once
#include <semaphore.h>
#include <stdint.h>
#include <stdexcept>
#include <atomic>

namespace MutexSpace
{
	using std::logic_error;
	using std::atomic_flag;
	using std::atomic_flag_test_and_set_explicit;
	using std::atomic_flag_clear_explicit;
	using std::memory_order_acquire;
	using std::memory_order_release;

	/*
	* 类关系：
	* ScopedLock,ReadScopedLock,WriteScopedLock为监视者系列，用于管理互斥锁系列
	* 事先创建互斥锁对象，在需要锁定时再用其创建监视者对象，监视者对象生命周期结束时自动解锁
	*/
	/*
	* 互斥锁系统调用方法：
	* 1.从互斥锁系列和监视者系列中分别选择合适的类，定义互斥锁对象
	* 2.再将互斥锁类作为监视者类的模板参数，用互斥锁对象创建监视者对象
	* 3.监视者对象一经创建便执行锁定功能，直到生命周期结束时自动解锁
	*/
	/*
	* 信号量调用方法：
	* 1.事先创建信号量对象
	* 2.需要阻塞进程时令信号量对象调用wait()方法
	* 3.需要继续运行时令信号量对象调用notify()方法
	*/
	
	//互斥锁系列
	class Mutex;		//通用互斥锁
	class NullMutex;	//空互斥锁，用于调试
	class Mutex_Read_Write;		//读/写互斥锁
	class NullMutex_Read_Write;	//空读/写互斥锁，用于调试
	class SpinLock;				//自旋锁，替换较低速的Mutex
	class CASLock;				//原子锁

	class Semaphore;			//信号量类

	//typedef SpinLock Mutex;
	

	//监视者系列

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

	



	//互斥锁系列

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
	//空互斥锁，用于调试
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
	//空读/写互斥锁，用于调试
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


	//自旋锁
	class SpinLock
	{
	public:
		SpinLock()
		{
			pthread_spin_init(&m_mutex,0);
		}
		~SpinLock()
		{
			pthread_spin_destroy(&m_mutex);
		}

		void lock()
		{
			pthread_spin_lock(&m_mutex);
		}

		void unlock()
		{
			pthread_spin_unlock(&m_mutex);
		}
	private:
		pthread_spinlock_t m_mutex;
	};

	//原子锁
	class CASLock
	{
	public:
		CASLock()
		{
			m_mutex.clear();
		}
		~CASLock()
		{

		}

		void lock()
		{
			while (atomic_flag_test_and_set_explicit(&m_mutex, memory_order_acquire))
			{

			}
		}

		void unlock()
		{
			atomic_flag_clear_explicit(&m_mutex, memory_order_release);
		}
	private:
		volatile atomic_flag m_mutex;
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
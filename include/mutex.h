#pragma once
#include <semaphore.h>
#include <stdint.h>
#include <stdexcept>
#include <atomic>
#include <pthread.h>

#include "noncopyable.h"

namespace MutexSpace
{
	using namespace NoncopyableSpace;

	using std::logic_error;
	using std::atomic_flag;
	using std::atomic_flag_test_and_set_explicit;
	using std::atomic_flag_clear_explicit;
	using std::memory_order_acquire;
	using std::memory_order_release;

	/*
	* ���ϵ��
	* ScopedLock,ReadScopedLock,WriteScopedLockΪ������ϵ�У����ڹ���������ϵ��
	* ���ȴ�����������������Ҫ����ʱ�����䴴�������߶�����ݻ������������߶����������ڽ���ʱ�Զ�����
	*/
	/*
	* ������ϵͳ���÷�����
	* 1.�ӻ�����ϵ�кͼ�����ϵ���зֱ�ѡ����ʵ��࣬���ȴ�������������
	* 2.�ٽ�����������Ϊ���������ģ��������û��������󴴽������߶���
	* 3.�����߶���һ��������ִ���������ܣ�ֱ���������ڽ���ʱ�Զ�����
	*/
	/*
	* �ź������÷�����
	* 1.���ȴ����ź�������
	* 2.��Ҫ��������ʱ���ź����������wait()����
	* 3.��Ҫ��������ʱ���ź����������notify()����
	*/
	
	//������ϵ��
	class Mutex;		//ͨ�û�����
	class NullMutex;	//�ջ����������ڵ���
	class Mutex_Read_Write;		//��/д������
	class NullMutex_Read_Write;	//�ն�/д�����������ڵ���
	class SpinLock;				//���������滻�ϵ��ٵ�Mutex
	class CASLock;				//ԭ����

	class Semaphore;			//�ź�����
	

	//������ϵ��

	//�����ӵ�ͨ�û�����
	template<class T>
	class ScopedLock
	{
	public:
		//����ScopedLock���󲢽�����������
		ScopedLock(T& mutex) :m_mutex(mutex)
		{
			m_mutex.lock();
			m_is_locked = true;
		}

		//����ScopedLock���󲢽�����������
		~ScopedLock()
		{
			unlock();
		}

		//����
		void lock()
		{
			//���������δ������������
			if (!m_is_locked)
			{
				m_mutex.lock();
				m_is_locked = true;
			}
		}

		//����
		void unlock()
		{
			//����������������������
			if (m_is_locked)
			{
				m_mutex.unlock();
				m_is_locked = false;
			}
		}
	private:
		T& m_mutex;			//������
		bool m_is_locked;	//����״̬
	};

	//�����ӵĶ�ȡ��
	template<class T>
	class ReadScopedLock
	{
	public:
		//����ReadScopedLock���󲢽���ȡ������
		ReadScopedLock(T& mutex) :m_mutex(mutex)
		{
			m_mutex.read_lock();
			m_locked = true;
		}

		//����ReadScopedLock���󲢽���ȡ������
		~ReadScopedLock()
		{
			unlock();
		}

		//����
		void lock()
		{
			//�����ȡ��δ������������
			if (!m_locked)
			{
				m_mutex.read_lock();
				m_locked = true;
			}
		}

		//����
		void unlock()
		{
			//�����ȡ���������������
			if (m_locked)
			{
				m_mutex.unlock();
				m_locked = false;
			}
		}
	private:
		T& m_mutex;		//��ȡ��
		bool m_locked;	//����״̬
	};

	//�����ӵ�д����
	template<class T>
	class WriteScopedLock
	{
	public:
		//����WriteScopedLock���󲢽�д��������
		WriteScopedLock(T& mutex) :m_mutex(mutex)
		{
			m_mutex.write_lock();
			m_locked = true;
		}

		//����WriteScopedLock���󲢽�д��������
		~WriteScopedLock()
		{
			unlock();
		}

		//����
		void lock()
		{
			//���д����δ������������
			if (!m_locked)
			{
				m_mutex.write_lock();
				m_locked = true;
			}
		}

		//����
		void unlock()
		{
			//���д�����������������
			if (m_locked)
			{
				m_mutex.unlock();
				m_locked = false;
			}
		}
	private:
		T& m_mutex;		//д����
		bool m_locked;	//����״̬
	};

	



	//������ϵ��

	//ͨ�û���������ֹ����
	class Mutex	: private Noncopyable
	{
	public:
		//����Mutex_Read_Write���󲢳�ʼ����/д��
		Mutex()
		{
			pthread_mutex_init(&m_mutex, nullptr);
		}

		//����Mutex_Read_Write�������ٶ�/д��
		~Mutex()
		{
			pthread_mutex_destroy(&m_mutex);
		}

		//����������
		void lock()
		{
			pthread_mutex_lock(&m_mutex);
		}

		//����������
		void unlock()
		{
			pthread_mutex_unlock(&m_mutex);
		}
	private:
		pthread_mutex_t m_mutex;	//������
	};
	//�ջ���������ֹ���ƣ����ڵ���
	class NullMutex :private Noncopyable
	{
	public:
		//ʲô������
		NullMutex() {}
		~NullMutex() {}
		//����������
		void lock() {}
		//����������
		void unlock() {}
	};


	//��/д����������ֹ����
	class Mutex_Read_Write :private Noncopyable
	{
	public:
		//����Mutex_Read_Write���󲢳�ʼ����/д��
		Mutex_Read_Write()
		{
			pthread_rwlock_init(&m_lock, nullptr);
		}

		//����Mutex_Read_Write�������ٶ�/д��
		~Mutex_Read_Write()
		{
			pthread_rwlock_destroy(&m_lock);
		}

		//������ȡ��
		void read_lock()
		{
			pthread_rwlock_rdlock(&m_lock);
		}

		//����д����
		void write_lock()
		{
			pthread_rwlock_wrlock(&m_lock);
		}

		//������/д��
		void unlock()
		{
			pthread_rwlock_unlock(&m_lock);
		}

	private:
		pthread_rwlock_t m_lock;	//��/д������
	};
	//�ն�/д����������ֹ����,���ڵ���
	class NullMutex_Read_Write :private Noncopyable
	{
	public:
		//ʲô������
		NullMutex_Read_Write() {}
		~NullMutex_Read_Write() {}
		//������ȡ��
		void read_lock() {}
		//����д����
		void write_lock() {}
		//������/д��
		void unlock() {}
	};


	//����������ֹ����
	class SpinLock :private Noncopyable
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

	//ԭ��������ֹ����
	class CASLock :private Noncopyable
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


	//�ź����࣬��ֹ����
	class Semaphore : private Noncopyable
	{
	public:
		//����Semaphore���󲢳�ʼ���ź���,countΪ�ź�����ʼֵ
		Semaphore(const uint32_t count = 0);
		//����Semaphore���������ź���
		~Semaphore();

		//�����߳�ֱ���ź�������0���������һ
		void wait();
		//���ź�����һ
		void notify();
	private:
		sem_t m_semaphore;		//�ź���
	};
}
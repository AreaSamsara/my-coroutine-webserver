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
	struct ReadScopedLock
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
	struct WriteScopedLock
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



	//��/д������
	class Mutex_Read_Write
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



	//�ź�����
	class Semaphore
	{
	public:
		//����Semaphore���󲢳�ʼ���ź���,countΪ�ź�����ʼֵ
		Semaphore(uint32_t count = 0);
		//����Semaphore���������ź���
		~Semaphore();

		//�����߳�ֱ���ź�������0���������һ
		void wait();
		//���ź�����һ
		void notify();
	private:
		//ɾ�����ƹ��캯��
		Semaphore(const Semaphore&) = delete;
		//ɾ���ƶ����캯��
		Semaphore(const Semaphore&&) = delete;
		//ɾ����ֵ�����
		Semaphore& operator=(const Semaphore&) = delete;
	private:
		sem_t m_semaphore;		//�ź���
	};



	//�߳���
	class Thread
	{
	public:
		Thread(function<void()> callback, const string& name);
		//����Thread���󲢽��߳�����Ϊ����״̬
		~Thread();

		//��ȡ˽�г�Ա
		pid_t getId()const { return m_id; }
		const string& getName()const { return m_name; }

		//�����������еĽ��̣���thread������̶��в���ʼִ��
		void join();

		//��ȡ�߳�ר����Thread��ָ�룬����Ϊ��̬�����Է��ʾ�̬����
		static Thread* getThis();
		//static const string& s_getName();
		//static void setName(const string& name);
	private:
		//ɾ�����ƹ��캯��
		Thread(const Thread&) = delete;
		//ɾ���ƶ����캯��
		Thread(const Thread&&) = delete;
		//ɾ����ֵ�����
		Thread& operator=(const Thread&) = delete;

		//���ݸ�pthread_create�������к���
		static void* run(void* arg);
	private:
		pthread_t m_thread = 0;		//�̣߳�Ĭ����Ϊ0��ʾ��δ������
		string m_name;				//�߳�����
		pid_t m_id = -1;			//�߳�id
		function<void()> m_callback;	//�̻߳ص�����
		
		Semaphore m_semaphore;		//�ź���

	public:
		static thread_local Thread* t_thread;	//�߳�ר����Thread��ָ�루�߳�ר�������������������߳�����������ʹ����ָ�룩
	};
}

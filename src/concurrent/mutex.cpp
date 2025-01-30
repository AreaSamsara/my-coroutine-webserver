#include "concurrent/mutex.h"

namespace MutexSpace
{
	// class Semaphore:public
	// ����Semaphore���󲢳�ʼ���ź���,countΪ�ź�����ʼֵ
	Semaphore::Semaphore(const uint32_t count)
	{
		// ��ʼ���ź������ɹ��򷵻�0�����򱨴�
		if (sem_init(&m_semaphore, 0, count) != 0) // ����0��ʾ�ڽ����ڲ�ʹ�ã�countΪ�ź�����ʼֵ
		{
			throw logic_error("sem_init error");
		}
	}
	// ����Semaphore���������ź���
	Semaphore::~Semaphore()
	{
		sem_destroy(&m_semaphore);
	}

	// �����߳�ֱ���ź�������0���������һ
	void Semaphore::wait()
	{
		// �����߳�ֱ���ź�������0�Ž����һ�����أ��ɹ�����0�����򱨴�
		if (sem_wait(&m_semaphore) != 0)
		{
			throw logic_error("sem_wait error");
		}
	}
	// ���ź�����һ
	void Semaphore::notify()
	{
		// ���ź�����һ���ɹ�����0�����򱨴�
		if (sem_post(&m_semaphore) != 0)
		{
			throw logic_error("sem_post error");
		}
	}
}
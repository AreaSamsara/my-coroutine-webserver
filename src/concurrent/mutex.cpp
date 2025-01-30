#include "concurrent/mutex.h"

namespace MutexSpace
{
	// class Semaphore:public
	// 创建Semaphore对象并初始化信号量,count为信号量初始值
	Semaphore::Semaphore(const uint32_t count)
	{
		// 初始化信号量，成功则返回0，否则报错
		if (sem_init(&m_semaphore, 0, count) != 0) // 参数0表示在进程内部使用，count为信号量初始值
		{
			throw logic_error("sem_init error");
		}
	}
	// 析构Semaphore对象并销毁信号量
	Semaphore::~Semaphore()
	{
		sem_destroy(&m_semaphore);
	}

	// 阻塞线程直到信号量大于0，并将其减一
	void Semaphore::wait()
	{
		// 阻塞线程直到信号量大于0才将其减一并返回，成功返回0，否则报错
		if (sem_wait(&m_semaphore) != 0)
		{
			throw logic_error("sem_wait error");
		}
	}
	// 将信号量加一
	void Semaphore::notify()
	{
		// 将信号量加一，成功返回0，否则报错
		if (sem_post(&m_semaphore) != 0)
		{
			throw logic_error("sem_post error");
		}
	}
}
#pragma once
#include "thread.h"

namespace ThreadSpace
{
	//class Thread:public
	Thread::Thread(const function<void()>& callback, const string& name) :m_callback(callback), m_name(name)
	{
		//������������Ϊ���ַ�������Ϊ"UNKNOW"
		if (name.empty())
		{
			m_name = "UNKNOW";
		}

		//�����̣߳�һ�������̱߳㿪ʼִ��
		int return_value = pthread_create(&m_thread, nullptr, &run, this);
		if (return_value != 0)		//����0��ʾ�����ɹ������򱨴�
		{
			//������־�¼�
			//__FILE__���ص�ǰ�ļ����ļ������Դ�·������__LINE__���ص�ǰ��������;elapseΪ����ֵ
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "pthread_create thread fail, return_value=" << return_value << " name=" << name;
			//ʹ��LoggerManager������Ĭ��logger�����־
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);

			throw logic_error("pthread_create error");
		}

		//�������̣߳��ȴ��ź�������0��run������ָ��ٽ������캯��
		m_semaphore.wait();
	}

	//����Thread���󲢽��߳�����Ϊ����״̬
	Thread::~Thread()
	{
		//����̴߳��ڣ����߳�����Ϊ����״̬
		if (m_thread)
		{
			pthread_detach(m_thread);
		}
	}

	//�����������еĽ��̣���thread�����̵߳ȴ�����
	void Thread::join()
	{
		//����̴߳��ڣ����̼߳����̵߳ȴ�����
		if (m_thread)
		{
			int return_value = pthread_join(m_thread,nullptr);
			if (return_value != 0)		//�ɹ�������̶����򷵻�0�����򱨴�
			{
				//������־�¼�
				//__FILE__���ص�ǰ�ļ����ļ������Դ�·������__LINE__���ص�ǰ��������;elapseΪ����ֵ
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "pthread_join thread fail, return_value=" << return_value << " name=" << m_name;
				throw logic_error("pthread_join error");
				//ʹ��LoggerManager������Ĭ��logger�����־
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			}

			//�߳���ִ����ϣ�����m_threadΪ0
			m_thread = 0;
		}
	}


	//class Thread:public static
	//��ȡ�߳�ר����Thread��ָ�룬����Ϊ��̬�����Է��ʾ�̬����
	Thread* Thread::getThis()
	{
		return t_thread;
	}


	//class Thread:private static
	//���ݸ�pthread_create�������к���
	void* Thread::run(void* arg)
	{
		//����Thread*���Ͳ�������ǰ�̶߳���
		Thread* thread = (Thread*)arg;

		//���þ�̬����t_thread
		t_thread = thread;

		//��ȡ�߳�id
		thread->m_id = UtilitySpace::GetThread_id();
		//�����߳����ƣ���������15���ַ����ڣ�
		pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

		//��thread�Ļص��������Ƶ��ֲ����������У�ȷ��thread������
		function<void()> callback;
		callback.swap(thread->m_callback);

		//���������Ѿ��������������ź���������̣߳������лص�����
		thread->m_semaphore.notify();
		callback();

		return 0;
	}


	//class Thread:public static variable
	thread_local Thread* Thread::t_thread = nullptr;	//�߳�ר����Thread��ָ�루�߳�ר�������������������߳�������������ʹ����ָ�룩
}

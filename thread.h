#pragma once
#include <functional>

#include "singleton.h"
#include "utility.h"
#include "log.h"
#include "noncopyable.h"

namespace ThreadSpace
{
	using namespace SingletonSpace;
	using namespace LogSpace;
	using namespace UtilitySpace;
	using namespace NoncopyableSpace;
	using std::function;

	/*
	* �߳�����÷�����
	* 1.������ִ�еĺ���������Ϊ�ص���������Thread���󣬼��ɿ�ʼ�̵߳�ִ��
	* 2.����Thread�����join()�����������������̣߳������̵߳ȴ����߳�ִ������ټ���
	*/

	//�߳��࣬��ֹ����
	class Thread :private Noncopyable
	{
	public:
		Thread(const function<void()>& callback, const string& name);
		//����Thread���󲢽��߳�����Ϊ����״̬
		~Thread();

		//��ȡ˽�г�Ա
		pid_t getId()const { return m_id; }
		const string& getName()const { return m_name; }

		//�����������еĽ��̣���thread����ȴ�����
		void join();
	public:
		//��ȡ�߳�ר����Thread��ָ�룬����Ϊ��̬�����Է��ʾ�̬����
		static Thread* getThis();
	private:
		//ɾ�����ƹ��캯��
		Thread(const Thread&) = delete;
		//ɾ���ƶ����캯��
		Thread(const Thread&&) = delete;
		//ɾ����ֵ�����
		Thread& operator=(const Thread&) = delete;
	private:
		//���ݸ�pthread_create�������к���
		static void* run(void* arg);
	public:
		static thread_local Thread* t_thread;	//�߳�ר����Thread��ָ�루�߳�ר�������������������߳�������������ʹ����ָ�룩
	private:
		pthread_t m_thread = 0;		//�̣߳�Ĭ����Ϊ0��ʾ��δ������
		string m_name;				//�߳�����
		pid_t m_id = -1;			//�߳�id
		function<void()> m_callback;	//�̻߳ص�����
		
		Semaphore m_semaphore;		//�ź���
	};
}

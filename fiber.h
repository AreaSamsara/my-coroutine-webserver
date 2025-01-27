#pragma once
#include <ucontext.h>
#include <atomic>

#include "thread.h"

namespace FiberSpace
{
	using std::enable_shared_from_this;
	using std::shared_ptr;
	using std::function;
	using std::atomic;

	/*
	* Э������÷�����һ����Э�̵�����ֱ�ӵ��ã���
	* 1.�����߳����״ε���Fiber::GetThis()��̬�����Դ���ջΪ�յ���Э�̣���Ϊ�ٿ���Э�̵�ý��
	* 2.�����߳��ڵ��ù��캯��������Э�̣�Ϊ�����ջ�ͻص���������ʹ��swapIn()�����л�����Э��ִ��
	* 3.Э�̵Ļص�����ִ������Ժ���Զ�ʹ��swapOut()�����л��ص�ǰ�̵߳���Э��
	* 4.*�����Ҫʵ��Э�̵�����л��������ڻص������ڲ�ʹ��swapOut()��YieldToReady()��YieldToHold()�ȷ���
	*/

	//Э���ࣨ���м̳���enable_shared_from_this�࣬��ʹ��shared_from_this()������
	class Fiber : public enable_shared_from_this<Fiber>		
	{
	public:
		//��ʾЭ��״̬��ö������
		enum State
		{
			//��ʼ��״̬
			INITIALIZE,
			//����״̬
			HOLD,
			//ִ��״̬
			EXECUTE,
			//��ֹ״̬
			TERMINAL,
			//׼��״̬
			READY,
			//�쳣״̬
			EXCEPT
		};
	public:
		//���ڴ�����Э�̵Ĺ��캯��
		Fiber(const function<void()>& callback, const size_t stacksize = 1024 * 1024);
		~Fiber();

		//����Э�̺�����״̬��������ȥ�ڴ���ٷ��䣬�ڿ��е��ѷ����ڴ���ֱ�Ӵ�����Э��
		void reset(const function<void()>& callback);
		//�л�����Э��ִ��
		void swapIn();
		//����Э���л�����̨
		void swapOut();

		//��ȡЭ��id
		uint64_t getId()const { return m_id; }

		//��ȡЭ��״̬
		State getState()const { return m_state; }
		//����Э��״̬
		void setState(const State state) { m_state = state; }
	public:
		//��ȡ��ǰЭ�̣������ڵ�һ�ε���ʱ������Э��
		static shared_ptr<Fiber> GetThis();

		//��Э���л�����̨������ΪReady״̬
		static void YieldToReady();
		//��Э���л�����̨������ΪHold״̬
		static void YieldToHold();

		//��ȡ��ǰЭ��id
		static uint64_t GetFiber_id();

		//Э�̵������к���
		static void MainFunction();
	private:
		//Ĭ�Ϲ��캯����˽�з��������ڳ��δ�����Э��ʱ����̬��GetThis()��������
		Fiber();
	public:
		//ÿ���߳�ר���ĵ�ǰЭ�̣��߳�ר�������������������߳�������������ʹ����ָ�룩
		static thread_local Fiber* t_fiber;
		//ÿ���߳�ר������Э��
		static thread_local shared_ptr <Fiber> t_main_fiber;
	private:
		//Э��id
		uint64_t m_id = 0;
		//Э��״̬
		State m_state = INITIALIZE;
		//Э��ջ��С��Ĭ��Ϊ0����Ϊ��Э�̵�ջ��СΪ0��
		uint32_t m_stacksize = 0;
		//Э��ջ
		void* m_stack = nullptr;
		//Э���ﾳ
		ucontext_t m_context;
		//Э�̻ص�����
		function<void()> m_callback;
	private:
		//��һ����Э�̵�id����̬ԭ�����ͣ���֤��д���̰߳�ȫ��
		static atomic<uint64_t> s_new_fiber_id;
		//Э����������̬ԭ�����ͣ���֤��д���̰߳�ȫ��
		static atomic<uint64_t> s_fiber_count;
	};
}
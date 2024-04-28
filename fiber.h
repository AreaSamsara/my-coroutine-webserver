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
	private:
		//Ĭ�Ϲ��캯����˽�з��������ڳ��δ�����Э��ʱ����̬��GetThis()��������
		Fiber();
	public:
		//���ڴ�����Э�̵Ĺ��캯��
		Fiber(function<void()> callback, size_t stacksize = 1024 * 1024);
		~Fiber();

		//����Э�̺�����״̬��������ȥ�ڴ���ٷ��䣬�ڿ��е��ѷ����ڴ���ֱ�Ӵ�����Э��
		void reset(function<void()> callback);
		//�л�����Э��ִ��
		void swapIn();
		//����Э���л�����̨
		void swapOut();

		//void call();
		//void back();

		//��ȡЭ��id
		uint64_t getId()const { return m_id; }

		//��ȡЭ��״̬
		State getState()const { return m_state; }
		//����Э��״̬
		void setState(State state) { m_state = state; }
	public:
		////���õ�ǰЭ��
		//static void SetThis(Fiber* fiber);
		//��ȡ��ǰЭ�̣������ڵ�һ�ε���ʱ������Э��
		static shared_ptr<Fiber> GetThis();

		//��Э���л�����̨������ΪReady״̬
		static void YieldTOReady();
		//��Э���л�����̨������ΪHold״̬
		static void YieldTOHold();

		////��ȡ��Э����
		//static uint64_t GetFiber_count();
		//��ȡ��ǰЭ��id
		static uint64_t GetFiber_id();

		//Э�̵������к���
		static void MainFunction();
		//static void CallerMainFunction();
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
	public:
		//ÿ���߳�ר���ĵ�ǰЭ�̣��߳�ר�������������������߳�����������ʹ����ָ�룩
		static thread_local Fiber* t_fiber;
		//ÿ���߳�ר������Э��
		static thread_local shared_ptr <Fiber> t_main_fiber;
	};

	//ջ�ڴ������
	class MallocStackAllocator
	{
	public:
		static void* Allocate(const size_t size)
		{
			return malloc(size);
		}

		static void Deallocate(void* memory, const size_t size)
		{
			return free(memory);
		}
	};
}
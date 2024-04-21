#pragma once
#include <ucontext.h>
#include "thread.h"

namespace FiberSpace
{
	using std::enable_shared_from_this;
	using std::shared_ptr;
	using std::function;

	class Fiber : public enable_shared_from_this<Fiber>
	{
	public:
		enum State
		{
			INIT,
			HOLD,
			EXEC,
			TERM,
			READY,
			EXCEPT
		};
	private:
		Fiber();
	public:
		Fiber(function<void()> callback, size_t stacksize = 1024 * 1024);
		~Fiber();

		//����Э�̺�����״̬
		//INIT��TERM
		void reset(function<void()> callback);
		//�л�����ǰЭ��ִ��
		void swapIn();
		//�л�����ִ̨��
		void swapOut();

		//��ȡЭ��id
		uint64_t getId()const { return m_id; }
	public:
		//���õ�ǰЭ��
		static void SetThis(Fiber* fiber);
		//��ȡ��ǰЭ��
		static shared_ptr<Fiber> GetThis();
		//Э���л�����̨������ΪReady״̬
		static void YieldTOReady();
		//Э���л�����̨������ΪHold״̬
		static void YieldTOHold();
		//��Э����
		static uint64_t TotalFibers();

		static void MainFunc();
		//��ȡ��ǰЭ��id
		static uint64_t GetFiber_id();

	private:
		uint64_t m_id = 0;
		uint32_t m_stacksize = 0;
		State m_state = INIT;

		ucontext_t m_context;
		void* m_stack = nullptr;

		function<void()> m_callback;
	};


	class MallocStackAllocator
	{
	public:
		static void* Allocate(size_t size)
		{
			return malloc(size);
		}

		static void Deallocate(void* vp, size_t size)
		{
			return free(vp);
		}
	};
}
#pragma once
#include "scheduler.h"

namespace IOManagerSpace
{
	using namespace SchedulerSpace;
	using std::string;

	class IOManager :public Scheduler
	{
	public:
		enum Event
		{
			NONE = 0x0,
			READ = 0x1,	//EPOLLIN
			WRITE = 0x4	//EPOLLOUT
		};
	private:
		class FileDescriptorContext
		{
		public:
			class EventContext
			{
			public:
				Scheduler* m_scheduler;		//�¼�ִ�е�scheduler
				shared_ptr<Fiber> m_fiber;	//�¼�Э��
				function<void()> m_callback;	//�¼��Ļص�����
			};
			EventContext& getContext(const Event event);
			void resetContext(EventContext& event_context);
			void triggerEvent(Event event);

			EventContext m_read;	//��ȡ�¼�
			EventContext m_write;	//д���¼�
			Event m_events = NONE;	//�Ѿ�ע����¼�
			Mutex m_mutex;			//������
			int m_file_descriptor;	//�¼������ľ��
		};
	public:
		IOManager(size_t thread_count, const bool use_caller = true, const string& name = "main_scheduler");
		virtual ~IOManager();

		//1:success	0:retry	-1:error
		int addEvent(const int file_description, const Event event, function<void()> callback = nullptr);
		//ɾ���¼�
		bool deleteEvent(const int file_description, const Event event);
		//ȡ���¼�
		bool cancelEvent(const int file_description, const Event event);

		bool cancelAllEvents(const int file_description);
	public:
		static IOManager* GetThis();
	protected:
		void tickle()override;
		bool is_completed()override;
		void idle()override;

		void contextResize(const size_t size);
	private:
		int m_epoll_file_descriptor = 0;
		int m_tickle_file_descriptors[2];
		//��ǰ�ȴ�ִ�е��¼�������ԭ�����ͣ���֤��д���̰߳�ȫ��
		atomic<size_t> m_pending_event_count = { 0 };
		//����������/д����
		Mutex_Read_Write m_mutex;
		vector<FileDescriptorContext*> m_file_descriptor_contexts;
	};
}
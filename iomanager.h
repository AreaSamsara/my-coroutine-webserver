#pragma once
#include "scheduler.h"
#include "timer.h"

namespace IOManagerSpace
{
	using namespace SchedulerSpace;
	using namespace TimerSpace;
	using std::string;

	class IOManager :public Scheduler,public TimerManager
	{
	public:
		//��ʾ�¼����͵�ö������
		enum Event
		{
			NONE = 0x0,
			READ = 0x1,	//EPOLLIN
			WRITE = 0x4	//EPOLLOUT
		};
	private:
		//�ļ��������ﾳ��
		class FileDescriptorContext
		{
		public:
			//�¼��ﾳ��
			class EventContext
			{
			public:
				Scheduler* m_scheduler;		//�¼�ִ�е�scheduler
				shared_ptr<Fiber> m_fiber;	//�¼�Э��
				function<void()> m_callback;	//�¼��Ļص�����
			};
			//��ȡ�¼���Ӧ���ﾳ
			EventContext& getContext(const Event event);
			//�����ﾳ
			void resetContext(EventContext& event_context);
			//�����¼�
			void triggerEvent(Event event);

			EventContext m_read;	//��ȡ�¼�
			EventContext m_write;	//д���¼�
			Event m_events = NONE;	//�Ѿ�ע����¼�
			Mutex m_mutex;			//������
			int m_file_descriptor;	//�¼��������ļ�������
		};
	public:
		IOManager(size_t thread_count, const bool use_caller = true, const string& name = "main_scheduler");
		virtual ~IOManager();

		//����¼����ɹ�����0��ʧ�ܷ���-1
		int addEvent(const int file_description, const Event event, function<void()> callback = nullptr);
		//ɾ���¼�
		bool deleteEvent(const int file_description, const Event event);
		//ȡ���¼�
		bool cancelEvent(const int file_description, const Event event);
		//ȡ�������¼�
		bool cancelAllEvents(const int file_description);
	public:
		static IOManager* GetThis();
	protected:
		//֪ͨ��������������
		void tickle()override;
		//�����Ƿ񿢹�
		bool is_completed()override;
		//�����Ƿ񿢹�
		bool is_completed(uint64_t & timeout);
		//����Э�̵Ļص�����
		void idle()override;

		virtual void onTimerInsertedAtFront() override;

		//�����ļ��������ﾳ������С
		void resizeFile_descriptor_contexts(const size_t size);
	private:
		//epoll�ļ�������
		int m_epoll_file_descriptor = 0;
		//ͨ�Źܵ�pipe�ļ�������,��һ��Ԫ����pipe�Ķ�ȡ�ˣ��ڶ���Ԫ����pipe��д��ˣ���˫���ܵ���
		int m_pipe_file_descriptors[2];

		//��ǰ�ȴ�ִ�е��¼�������ԭ�����ͣ���֤��д���̰߳�ȫ��
		atomic<size_t> m_pending_event_count = { 0 };
		//����������/д����
		Mutex_Read_Write m_mutex;
		//�ļ����������ﾳ����
		vector<FileDescriptorContext*> m_file_descriptor_contexts;
	};
}
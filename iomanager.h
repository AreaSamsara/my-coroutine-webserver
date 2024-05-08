#pragma once
#include "scheduler.h"
#include "timer.h"

namespace IOManagerSpace
{
	using namespace SchedulerSpace;
	using namespace TimerSpace;
	using std::string;

	//IO�����ߣ����м̳��Ե�������
	class IOManager :public Scheduler
	{
	public:
		//��ʾ�¼����͵�ö������
		enum EventType
		{
			NONE = 0x00,
			READ = 0x01,	//EPOLLIN
			WRITE = 0x04	//EPOLLOUT
		};
	private:
		//�ļ��������¼���
		class FileDescriptorEvent
		{
		public:
			//�����¼����ͻ�ȡ��Ӧ�Ļص�����
			function<void()>& getCallback(const EventType event_type);
			//�����¼�
			void triggerEvent(const EventType event_type);
		public:
			int m_file_descriptor;				//�¼��������ļ�������
			function<void()> m_read_callback;	//��ȡ�¼�
			function<void()> m_write_callback;	//д���¼�
			EventType m_registered_event_types = NONE;		//�Ѿ�ע����¼����ͣ���ʼ��ΪNONE
			Mutex m_mutex;						//������
		};
	public:
		IOManager(size_t thread_count, const bool use_caller = true, const string& name = "main_scheduler");
		virtual ~IOManager();

		//��ȡ��ʱ��������
		shared_ptr<TimerManager> getTimer_manager()const { return m_timer_manager; }

		//����¼����ļ��������ϣ��ɹ�����0��ʧ�ܷ���-1
		int addEvent(const int file_description, const EventType event, function<void()> callback);
		//ɾ���ļ��������ϵĶ�Ӧ�¼�
		bool deleteEvent(const int file_description, const EventType event);
		//���㣨������ɾ�����ļ��������ϵĶ�Ӧ�¼�
		bool settleEvent(const int file_description, const EventType event);
		//���㣨������ɾ�����ļ��������ϵ������¼�
		bool cancelAllEvents(const int file_description);

		//��Ӷ�ʱ����������Ҫʱ֪ͨ������������
		void addTimer(shared_ptr<Timer> timer);
	protected:
		//֪ͨ��������������
		void tickle()override;
		//�����Ƿ񿢹�
		bool is_completed()override;
		//����Э�̵Ļص�����
		void idle()override;

		//�����ļ��������¼�������С
		void resizeFile_descriptor_events(const size_t size);
	public:
		//��̬��������ȡ��ǰIO������
		static IOManager* GetThis() { return dynamic_cast<IOManager*>(Scheduler::GetThis()); }
	private:
		//epoll�ļ�������
		int m_epoll_file_descriptor = 0;
		//ͨ�Źܵ�pipe�ļ�������,��һ��Ԫ����pipe�Ķ�ȡ�ˣ��ڶ���Ԫ����pipe��д��ˣ���˫���ܵ���
		int m_pipe_file_descriptors[2];

		//��ǰ�ȴ�ִ�е��¼�������ԭ�����ͣ���֤��д���̰߳�ȫ��
		atomic<size_t> m_pending_event_count = { 0 };
		//����������/д����
		Mutex_Read_Write m_mutex;

		//�ļ��������¼�����
		//vector<FileDescriptorEvent*> m_file_descriptor_events;
		vector<shared_ptr<FileDescriptorEvent>> m_file_descriptor_events;

		//��ʱ��������
		shared_ptr<TimerManager> m_timer_manager;
	};
}
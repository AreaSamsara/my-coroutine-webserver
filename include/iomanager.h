#pragma once
#include "scheduler.h"
#include "timer.h"

namespace IOManagerSpace
{
	using namespace SchedulerSpace;
	using namespace TimerSpace;
	using std::string;

	/*
	* ���ϵ��
	* FileDescriptorEvent���Ǳ�IOManager�������˽���࣬���ļ����������¼���־�ͻص��������
	* FileDescriptorEvent���ڲ�ֻ�ж�ȡ���޸�˽�г�Ա�ķ����ʹ����¼��ķ������󲿷ָ��ӷ�����λ��IOManager���ڲ�
	* IOManager���ڲ���װ�ж���ļ��������¼����������Լ���������ļ��������¼��ķ���
	* IOManager��ͨ��FileDescriptorEvent��������epollϵͳ���н�������ͨ��TimerManager�ദ����ʱ��
	*/
	/*
	* IO����������÷�����
	* 1.�ȵ��ù��캯������IOManager���󣬿��Խ��俴��һ�������Scheduler����
	* 2.���캯���ڲ����Զ�����epoll�Ͷ�ʱ�������ߣ����߷ֱ��epoll�¼��Ͷ�ʱ���¼����й���
	* 3.���캯���ڲ�Ĭ�ϵ���start()�����������������
	* 4.(1)��Scheduler����һ�������Ե���schedule()��������Ҫ��ɵ�����Э�̻��߻ص�����������������У��Դ��ַ�ʽ���ȵ�����ύ��epoll����
	*   (2)������ͨ��addTimer()���������趨�����񣨻ص��������Ķ�ʱ�����Դ��ַ�ʽ���ȵ�����ύ�ɶ�ʱ�������ߴ���
	* 5.��������Ĭ�ϵ���stop()����ֹͣ���������ڵȴ�epoll������е����������Լ����еĶ�ʱ������������Ժ�IO������ֹͣ��
	*/

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
			Task& getTask(const EventType event_type)
			{
				switch (event_type)
				{
				case READ:
					return m_read_task;
				case WRITE:
					return m_write_task;
				default:
					//shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
					shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
					Assert(log_event, "getTask");
				}
			}
			//���¼�����ע���¼���ɾ����������֮
			void triggerEvent(const EventType event_type);
		public:
			int m_file_descriptor;				//�¼��������ļ�������
			Task m_read_task;					//��ȡ����
			Task m_write_task;					//д������
			EventType m_registered_event_types = NONE;		//�Ѿ�ע����¼����ͣ���ʼ��ΪNONE
			Mutex m_mutex;						//������
		};
	public:
		IOManager(size_t thread_count = 1, const bool use_caller = true, const string& name = "main_scheduler");
		virtual ~IOManager();

		//��ȡ��ʱ��������
		shared_ptr<TimerManager> getTimer_manager()const { return m_timer_manager; }

		//�����¼����ļ���������
		bool addEvent(const int file_description, const EventType event, function<void()> callback);		//�ڲ�Ҫ��callback����swap�������ʲ��ܼ�const&
		//ɾ���ļ��������ϵĶ�Ӧ�¼�
		bool deleteEvent(const int file_description, const EventType event);
		//���㣨������ɾ�����ļ��������ϵĶ�Ӧ�¼�
		bool settleEvent(const int file_description, const EventType event);
		//���㣨������ɾ�����ļ��������ϵ������¼�
		bool settleAllEvents(const int file_description);

		//���Ӷ�ʱ����������Ҫʱ֪ͨ������������
		void addTimer(const shared_ptr<Timer> timer);
	public:
		//��̬��������ȡ��ǰIO������
		static IOManager* GetThis() { return dynamic_cast<IOManager*>(Scheduler::GetThis()); }
	protected:
		//֪ͨ��������������
		void tickle()override;
		//�����Ƿ񿢹�
		bool isCompleted()override;
		//����Э�̵Ļص�����
		void idle()override;

		//�����ļ��������¼�������С
		void resizeFile_descriptor_events(const size_t size);
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
		vector<shared_ptr<FileDescriptorEvent>> m_file_descriptor_events;

		//��ʱ��������
		shared_ptr<TimerManager> m_timer_manager;
	};
}
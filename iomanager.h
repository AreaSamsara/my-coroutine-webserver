#pragma once
#include "scheduler.h"
#include "timer.h"

namespace IOManagerSpace
{
	using namespace SchedulerSpace;
	using namespace TimerSpace;
	using std::string;

	//IO管理者，公有继承自调度器类
	class IOManager :public Scheduler
	{
	public:
		//表示事件类型的枚举类型
		enum EventType
		{
			NONE = 0x00,
			READ = 0x01,	//EPOLLIN
			WRITE = 0x04	//EPOLLOUT
		};
	private:
		//文件描述符事件类
		class FileDescriptorEvent
		{
		public:
			//根据事件类型获取对应的回调函数
			function<void()>& getCallback(const EventType event_type);
			//触发事件
			void triggerEvent(const EventType event_type);
		public:
			int m_file_descriptor;				//事件关联的文件描述符
			function<void()> m_read_callback;	//读取事件
			function<void()> m_write_callback;	//写入事件
			EventType m_registered_event_types = NONE;		//已经注册的事件类型，初始化为NONE
			Mutex m_mutex;						//互斥锁
		};
	public:
		IOManager(size_t thread_count, const bool use_caller = true, const string& name = "main_scheduler");
		virtual ~IOManager();

		//获取定时器管理者
		shared_ptr<TimerManager> getTimer_manager()const { return m_timer_manager; }

		//添加事件到文件描述符上，成功返回0，失败返回-1
		int addEvent(const int file_description, const EventType event, function<void()> callback);
		//删除文件描述符上的对应事件
		bool deleteEvent(const int file_description, const EventType event);
		//结算（触发并删除）文件描述符上的对应事件
		bool settleEvent(const int file_description, const EventType event);
		//结算（触发并删除）文件描述符上的所有事件
		bool cancelAllEvents(const int file_description);

		//添加定时器，并在需要时通知调度器有任务
		void addTimer(shared_ptr<Timer> timer);
	protected:
		//通知调度器有任务了
		void tickle()override;
		//返回是否竣工
		bool is_completed()override;
		//空闲协程的回调函数
		void idle()override;

		//重置文件描述符事件容器大小
		void resizeFile_descriptor_events(const size_t size);
	public:
		//静态方法，获取当前IO管理者
		static IOManager* GetThis() { return dynamic_cast<IOManager*>(Scheduler::GetThis()); }
	private:
		//epoll文件描述符
		int m_epoll_file_descriptor = 0;
		//通信管道pipe文件描述符,第一个元素是pipe的读取端，第二个元素是pipe的写入端（半双工管道）
		int m_pipe_file_descriptors[2];

		//当前等待执行的事件数量（原子类型，保证读写的线程安全）
		atomic<size_t> m_pending_event_count = { 0 };
		//互斥锁（读/写锁）
		Mutex_Read_Write m_mutex;

		//文件描述符事件容器
		//vector<FileDescriptorEvent*> m_file_descriptor_events;
		vector<shared_ptr<FileDescriptorEvent>> m_file_descriptor_events;

		//定时器管理者
		shared_ptr<TimerManager> m_timer_manager;
	};
}
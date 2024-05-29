#pragma once
#include "scheduler.h"
#include "timer.h"

namespace IOManagerSpace
{
	using namespace SchedulerSpace;
	using namespace TimerSpace;
	using std::string;

	/*
	* 类关系：
	* FileDescriptorEvent类是被IOManager类包含的私有类，由文件描述符、事件标志和回调函数组成
	* FileDescriptorEvent类内部只有读取或修改私有成员的方法和触发事件的方法，大部分复杂方法都位于IOManager类内部
	* IOManager类内部有装有多个文件描述符事件的容器，以及多个操纵文件描述符事件的方法
	* IOManager类通过FileDescriptorEvent类容器与epoll系统进行交互，而通过TimerManager类处理定时器
	*/
	/*
	* IO管理者类调用方法：
	* 1.先调用构造函数创建IOManager对象，可以将其看作一种特殊的Scheduler对象
	* 2.构造函数内部会自动创建epoll和定时器管理者，二者分别对epoll事件和定时器事件进行管理
	* 3.构造函数内部默认调用start()方法启动任务调度器
	* 4.(1)和Scheduler对象一样，可以调用schedule()方法将需要完成的任务（协程或者回调函数）加入任务队列，以此种方式调度的任务会交由epoll处理
	*   (2)还可以通过addTimer()方法添加设定好任务（回调函数）的定时器，以此种方式调度的任务会交由定时器管理者处理
	* 5.析构函数默认调用stop()方法停止调度器，在等待epoll任务队列的所有任务以及所有的定时器都处理完毕以后，IO管理者停止。
	*/

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
			Task& getTask(const EventType event_type)
			{
				switch (event_type)
				{
				case READ:
					return m_read_task;
				case WRITE:
					return m_write_task;
				default:
					shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
					Assert(log_event, "getTask");
				}
			}
			//将事件从已注册事件中删除，并触发之
			void triggerEvent(const EventType event_type);
		public:
			int m_file_descriptor;				//事件关联的文件描述符
			Task m_read_task;					//读取任务
			Task m_write_task;					//写入任务
			EventType m_registered_event_types = NONE;		//已经注册的事件类型，初始化为NONE
			Mutex m_mutex;						//互斥锁
		};
	public:
		IOManager(size_t thread_count = 1, const bool use_caller = true, const string& name = "main_scheduler");
		virtual ~IOManager();

		//获取定时器管理者
		shared_ptr<TimerManager> getTimer_manager()const { return m_timer_manager; }

		//添加事件到文件描述符上
		bool addEvent(const int file_description, const EventType event, function<void()> callback);		//内部要用callback调用swap函数，故不能加const&
		//删除文件描述符上的对应事件
		bool deleteEvent(const int file_description, const EventType event);
		//结算（触发并删除）文件描述符上的对应事件
		bool settleEvent(const int file_description, const EventType event);
		//结算（触发并删除）文件描述符上的所有事件
		bool settleAllEvents(const int file_description);

		//添加定时器，并在需要时通知调度器有任务
		void addTimer(const shared_ptr<Timer> timer);
	public:
		//静态方法，获取当前IO管理者
		static IOManager* GetThis() { return dynamic_cast<IOManager*>(Scheduler::GetThis()); }
	protected:
		//通知调度器有任务了
		void tickle()override;
		//返回是否竣工
		bool isCompleted()override;
		//空闲协程的回调函数
		void idle()override;

		//重设文件描述符事件容器大小
		void resizeFile_descriptor_events(const size_t size);
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
		vector<shared_ptr<FileDescriptorEvent>> m_file_descriptor_events;

		//定时器管理者
		shared_ptr<TimerManager> m_timer_manager;
	};
}
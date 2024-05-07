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
		//表示事件类型的枚举类型
		enum Event
		{
			NONE = 0x0,
			READ = 0x1,	//EPOLLIN
			WRITE = 0x4	//EPOLLOUT
		};
	private:
		//文件描述符语境类
		class FileDescriptorContext
		{
		public:
			//事件语境类
			class EventContext
			{
			public:
				Scheduler* m_scheduler;		//事件执行的scheduler
				shared_ptr<Fiber> m_fiber;	//事件协程
				function<void()> m_callback;	//事件的回调函数
			};
			//获取事件对应的语境
			EventContext& getContext(const Event event);
			//重置语境
			void resetContext(EventContext& event_context);
			//触发事件
			void triggerEvent(Event event);

			EventContext m_read;	//读取事件
			EventContext m_write;	//写入事件
			Event m_events = NONE;	//已经注册的事件
			Mutex m_mutex;			//互斥锁
			int m_file_descriptor;	//事件关联的文件描述符
		};
	public:
		IOManager(size_t thread_count, const bool use_caller = true, const string& name = "main_scheduler");
		virtual ~IOManager();

		//添加事件，成功返回0，失败返回-1
		int addEvent(const int file_description, const Event event, function<void()> callback = nullptr);
		//删除事件
		bool deleteEvent(const int file_description, const Event event);
		//取消事件
		bool cancelEvent(const int file_description, const Event event);
		//取消所有事件
		bool cancelAllEvents(const int file_description);
	public:
		static IOManager* GetThis();
	protected:
		//通知调度器有任务了
		void tickle()override;
		//返回是否竣工
		bool is_completed()override;
		//空闲协程的回调函数
		void idle()override;

		//virtual void onTimerInsertedAtFront() override;

		//重置文件描述符语境容器大小
		void resizeFile_descriptor_contexts(const size_t size);
	private:
		//epoll文件描述符
		int m_epoll_file_descriptor = 0;
		//通信管道pipe文件描述符,第一个元素是pipe的读取端，第二个元素是pipe的写入端（半双工管道）
		int m_pipe_file_descriptors[2];

		//当前等待执行的事件数量（原子类型，保证读写的线程安全）
		atomic<size_t> m_pending_event_count = { 0 };
		//互斥锁（读/写锁）
		Mutex_Read_Write m_mutex;
		//文件描述符的语境容器
		vector<FileDescriptorContext*> m_file_descriptor_contexts;
	};
}
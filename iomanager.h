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
				Scheduler* m_scheduler;		//事件执行的scheduler
				shared_ptr<Fiber> m_fiber;	//事件协程
				function<void()> m_callback;	//事件的回调函数
			};
			EventContext& getContext(const Event event);
			void resetContext(EventContext& event_context);
			void triggerEvent(Event event);

			EventContext m_read;	//读取事件
			EventContext m_write;	//写入事件
			Event m_events = NONE;	//已经注册的事件
			Mutex m_mutex;			//互斥锁
			int m_file_descriptor;	//事件关联的句柄
		};
	public:
		IOManager(size_t thread_count, const bool use_caller = true, const string& name = "main_scheduler");
		virtual ~IOManager();

		//1:success	0:retry	-1:error
		int addEvent(const int file_description, const Event event, function<void()> callback = nullptr);
		//删除事件
		bool deleteEvent(const int file_description, const Event event);
		//取消事件
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
		//当前等待执行的事件数量（原子类型，保证读写的线程安全）
		atomic<size_t> m_pending_event_count = { 0 };
		//互斥锁（读/写锁）
		Mutex_Read_Write m_mutex;
		vector<FileDescriptorContext*> m_file_descriptor_contexts;
	};
}
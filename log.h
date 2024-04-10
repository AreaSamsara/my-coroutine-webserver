#pragma once
#include "global.h"
namespace SylarSpace
{
	//日志事件
	class LogEvent
	{
	public:
		LogEvent();

		const string getFilename()const { return m_filename; }
		int32_t getLine()const { return m_line; }
		uint32_t getElapse()const { return m_elapse; }
		uint32_t getThread_id()const { return m_thread_id; }
		uint32_t getFiber_id()const { return m_fiber_id; }
		uint64_t getTime()const { return m_time; }
		const string& getContent()const { return m_content; }
	private:
		const string m_filename= nullptr;	//文件名
		int32_t m_line = 0;					//行号
		uint32_t m_elapse = 0;				//程序启动至今的毫秒数
		uint32_t m_thread_id = 0;			//线程id
		uint32_t m_fiber_id = 0;				//协程id
		uint64_t m_time;					//时间戳
		string m_content;
	};

	//日志级别
	class LogLevel
	{
	public:
		enum Level
		{
			DEBUG = 1,
			INFO = 2,
			WARN = 3,
			ERROR = 4,
			FATAL = 5
		};
		static const string ToString(Level level);
	};

	//日志输出器
	class Logger
	{
	public:
		Logger(const string& name = "root");
		void log(LogLevel::Level level, shared_ptr<LogEvent> event);

		//各个日志级别对应的日志输出函数
		void debug(shared_ptr<LogEvent> event);
		void info(shared_ptr<LogEvent> event);
		void warn(shared_ptr<LogEvent> event);
		void error(shared_ptr<LogEvent> event);
		void fatal(shared_ptr<LogEvent> event);

		//添加或删除Appender
		void addAppender(shared_ptr<LogAppender> appender);
		void deleteAppender(shared_ptr<LogAppender> appender);

		//读取或修改日志等级
		LogLevel::Level getlevel()const { return m_level; }
		void setLevel(LogLevel::Level level) { m_level = level; }

		//读取logger名称
		const string& getName()const { return m_name; }
	private:
		string m_name;		//logger名称
		LogLevel::Level m_level;	//满足该日志级别才输出日志
		list<shared_ptr<LogAppender>> m_appenders;	//appender集合
	};

	//日志输出地址
	class LogAppender
	{
	public:
		virtual ~LogAppender(){}
		virtual void log(shared_ptr<Logger> logger,LogLevel::Level level, shared_ptr<LogEvent> event) = 0;	//纯虚函数，子类必须实现
		
		//读取或修改formatter
		shared_ptr<LogFormatter> getFormatter()const { return m_formatter; }
		void setFormatter(shared_ptr<LogFormatter> formatter) { m_formatter = formatter; }
	protected:
		LogLevel::Level m_level;
		shared_ptr<LogFormatter> m_formatter;
	};

	//输出到控制台的Appender（公有继承自LogAppender）
	class StdoutLogAppender:public LogAppender
	{
	public:
		virtual void log(shared_ptr<Logger> logger,LogLevel::Level level, shared_ptr<LogEvent> event) override;
	private:
	};

	//输出到文件的Appender（公有继承自LogAppender）
	class FileLogAppender :public LogAppender
	{
	public:
		FileLogAppender(const string& filename);
		virtual void log(shared_ptr<Logger> logger,LogLevel::Level level, shared_ptr<LogEvent> event) override;
		//重新打开文件，文件打开成功返回true
		bool reopen();
	private:
		string m_filename;
		ostream m_filestream;
	};

	//日志格式器
	class LogFormatter
	{
	public:
		LogFormatter(const string& pattern);
		//%t	%thread_id%m%n
		string format(shared_ptr<Logger> logger,LogLevel::Level level,shared_ptr<LogEvent> event);
	private:
		class FormatItem
		{
		public:
			FormatItem(const string& format = "");
			virtual ~FormatItem();
			virtual void format(shared_ptr<Logger> logger,ostream& os, LogLevel::Level level,shared_ptr<LogEvent> event) = 0;
		};
		class MessageFormatItem:public FormatItem
		{
		public:
			virtual void format(shared_ptr<Logger> logger,ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event) override;
		};
		class LevelFormatItem :public FormatItem
		{
		public:
			virtual void format(shared_ptr<Logger> logger,ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event) override;
		};
		class ElapseFormatItem :public FormatItem
		{
		public:
			virtual void format(shared_ptr<Logger> logger,ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event) override;
		};
		class FilenameFormatItem :public FormatItem
		{
		public:
			virtual void format(shared_ptr<Logger> logger,ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event) override;
		};
		class Thread_idFormatItem :public FormatItem
		{
		public:
			virtual void format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event) override;
		};
		class Fiber_idFormatItem :public FormatItem
		{
		public:
			virtual void format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event) override;
		};
		class DataTimeFormatItem :public FormatItem
		{
		public:
			DataTimeFormatItem(const string& format = "%Y:%m:%d %H:%M:%S");
			virtual void format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event) override;
		private:
			string m_format;
		};
		class LineFormatItem :public FormatItem
		{
		public:
			virtual void format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event) override;
		};
		class EnterFormatItem :public FormatItem
		{
		public:
			virtual void format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event) override;
		};
		class StringFormatItem :public FormatItem
		{
		public:
			StringFormatItem(const string& str);
			virtual void format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event) override;
		private:
			string m_string;
		};

		void initialize();
	private:
		string m_pattern;
		vector<shared_ptr<FormatItem>> m_items;
	};
}


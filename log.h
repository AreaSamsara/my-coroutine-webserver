#pragma once
#include "global.h"
namespace SylarSpace
{
	/*
	* 类关系：
	* 先创建LogEvent作为日志事件，
	* Logger类（包含多个LogAppender）调用所有LogAppender对象，
	* LogAppender调用LogFormatter解析日志格式并输出LogEvent
	*/
	class LogLevel;			//日志级别
	class LogEvent;			//日志事件
	class LogAppender;		//日志输出地址
	class LogFormatter;		//日志格式器



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
		//将Level共用体转化为字符串（声明为静态方法使得其可以在未创建LogLevel类对象时被调用）
		static const string ToString(const Level level);
	};



	//日志事件
	class LogEvent
	{
	public:
		LogEvent(const string& filename,const string& threadname,const int32_t line,const uint32_t elapse,
			const uint32_t thread_id, const uint64_t fiber_id, const uint64_t time);

		//设置stringstream
		void setSstream(const string& str);

		//获取私有变量
		const string getFilename()const { return m_filename; }
		const string getThreadname()const { return m_threadname; }
		int32_t getLine()const { return m_line; }
		uint32_t getElapse()const { return m_elapse; }
		uint32_t getThread_id()const { return m_thread_id; }
		uint32_t getFiber_id()const { return m_fiber_id; }
		uint64_t getTime()const { return m_time; }
		const string getContent()const { return m_sstream.str(); }
		const stringstream& getSstream()const { return m_sstream; }

	private:
		const string m_filename;			//文件名（自带路径）
		const string m_threadname = 0;		//线程名称
		int32_t m_line = 0;					//行号
		uint32_t m_elapse = 0;				//程序启动至今的毫秒数
		uint32_t m_thread_id = 0;			//线程id
		uint32_t m_fiber_id = 0;			//协程id
		uint64_t m_time;					//时间戳
		stringstream m_sstream;				//事件内容的stringstream
	};



	//日志输出器
	class Logger
	{
	public:
		Logger(const string& name = Default_LoggerName);	//设置默认日志名称Default_LoggerName

		//日志输出函数
		void log(const LogLevel::Level level,const shared_ptr<const LogEvent> event);

		//添加或删除Appender
		void addAppender(const shared_ptr<LogAppender> appender);	//内含对appender的formatter的设置，故LogAppender不声明为const
		void deleteAppender(const shared_ptr<const LogAppender> appender);

		//读取或修改日志等级
		LogLevel::Level getlevel()const { return m_level; }
		void setLevel(const LogLevel::Level level) { m_level = level; }

		//读取logger名称
		const string& getName()const { return m_name; }

	private:
		string m_name;		//logger名称
		LogLevel::Level m_level;	//满足该日志级别才输出日志
		list<shared_ptr<LogAppender>> m_appenders;	//appender集合
		shared_ptr<LogFormatter> m_formatter;		//日志格式器
	public:
		//默认日志格式模式
		const static string Default_FormatPattern;
		//默认日志名称
		const static string Default_LoggerName;
		//默认日志时间模式
		const static string Default_DataTimePattern;
	};



	//日志输出地址
	class LogAppender
	{
	public:
		LogAppender(const LogLevel::Level level = LogLevel::DEBUG, const string& logger_name = Logger::Default_LoggerName);
		virtual ~LogAppender() {}		//父类的析构函数应该设置为虚函数
		//日志输出函数，由 Logger::log调用（纯虚函数，子类必须实现）
		virtual void log(const string& logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event) = 0;

		//读取或修改formatter
		shared_ptr<LogFormatter> getFormatter()const { return m_formatter; }
		void setFormatter(const shared_ptr<const LogFormatter> formatter) { m_formatter = formatter; }

		//读取或修改LogLevel
		LogLevel::Level getLevel()const { return m_level; }
		void setLevel(const LogLevel::Level level) { m_level = level; }

	protected:
		string m_logger_name;		//日志名称
		LogLevel::Level m_level;	//日志级别
		shared_ptr<LogFormatter> m_formatter;	//日志格式器
	};

	//输出到控制台的Appender（公有继承自LogAppender）
	class StdoutLogAppender :public LogAppender
	{
	public:
		StdoutLogAppender(const LogLevel::Level level = LogLevel::DEBUG, const string& logger_name= Logger::Default_LoggerName);

		//日志输出函数，由 Logger::log调用（重写LogAppender的相应方法）
		virtual void log(const string& logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event) override;
	};

	//输出到文件的Appender（公有继承自LogAppender）
	class FileLogAppender :public LogAppender
	{
	public:
		FileLogAppender(const LogLevel::Level level = LogLevel::DEBUG, const string& logger_name = Logger::Default_LoggerName,const string& filename="./log.txt");	//设置默认日志地址文件名为"./log.txt"

		//日志输出函数，由 Logger::log调用（重写LogAppender的相应方法）
		virtual void log(const string& logger_name,const LogLevel::Level level, const shared_ptr<const LogEvent> event) override;

		//重新打开文件，文件打开成功返回true
		bool reopen();

	private:
		string m_filename;		//日志地址文件名
		ofstream m_filestream;	//日志地址文件流
	};



	//日志格式器
	class LogFormatter
	{
		//解析规则：
		//%m 消息体
		//%p level
		//%r 启动后时间
		//%c 日志名称
		//%t 线程id
		//%n 回车换行
		//%d 时间
		//%f 文件名（含路径）
		//%l 行号
		//%T tab
		//%N 线程名称
		// 常用模式示例：
		// "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
		// "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
	public:
		LogFormatter(const string& pattern);

		//formatter主函数，由Appender调用
		string format(const string& logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event);

	private:

		//按照日志模式（pattern）初始化日志格式
		void initialize();

		//输出单段被解析的日志（const string& format为花括号中的内容）
		string write_piece(const string& logger_name,const LogLevel::Level level,const shared_ptr<const LogEvent> event,const string& mode="",const string& format="");
	private:
		string m_pattern;	//日志的模式，由Logger类初始化
		vector<pair<string,string>> m_modes_and_formats;	//被解析的单段日志模式集合,前者为解析'%'得到的模式，后者为'{}'内的内容
	};
}


#pragma once
#include "global.h"
namespace SylarSpace
{
	//��־�¼�
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
		const string m_filename= nullptr;	//�ļ���
		int32_t m_line = 0;					//�к�
		uint32_t m_elapse = 0;				//������������ĺ�����
		uint32_t m_thread_id = 0;			//�߳�id
		uint32_t m_fiber_id = 0;				//Э��id
		uint64_t m_time;					//ʱ���
		string m_content;
	};

	//��־����
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

	//��־�����
	class Logger
	{
	public:
		Logger(const string& name = "root");
		void log(LogLevel::Level level, shared_ptr<LogEvent> event);

		//������־�����Ӧ����־�������
		void debug(shared_ptr<LogEvent> event);
		void info(shared_ptr<LogEvent> event);
		void warn(shared_ptr<LogEvent> event);
		void error(shared_ptr<LogEvent> event);
		void fatal(shared_ptr<LogEvent> event);

		//��ӻ�ɾ��Appender
		void addAppender(shared_ptr<LogAppender> appender);
		void deleteAppender(shared_ptr<LogAppender> appender);

		//��ȡ���޸���־�ȼ�
		LogLevel::Level getlevel()const { return m_level; }
		void setLevel(LogLevel::Level level) { m_level = level; }

		//��ȡlogger����
		const string& getName()const { return m_name; }
	private:
		string m_name;		//logger����
		LogLevel::Level m_level;	//�������־����������־
		list<shared_ptr<LogAppender>> m_appenders;	//appender����
	};

	//��־�����ַ
	class LogAppender
	{
	public:
		virtual ~LogAppender(){}
		virtual void log(shared_ptr<Logger> logger,LogLevel::Level level, shared_ptr<LogEvent> event) = 0;	//���麯�����������ʵ��
		
		//��ȡ���޸�formatter
		shared_ptr<LogFormatter> getFormatter()const { return m_formatter; }
		void setFormatter(shared_ptr<LogFormatter> formatter) { m_formatter = formatter; }
	protected:
		LogLevel::Level m_level;
		shared_ptr<LogFormatter> m_formatter;
	};

	//���������̨��Appender�����м̳���LogAppender��
	class StdoutLogAppender:public LogAppender
	{
	public:
		virtual void log(shared_ptr<Logger> logger,LogLevel::Level level, shared_ptr<LogEvent> event) override;
	private:
	};

	//������ļ���Appender�����м̳���LogAppender��
	class FileLogAppender :public LogAppender
	{
	public:
		FileLogAppender(const string& filename);
		virtual void log(shared_ptr<Logger> logger,LogLevel::Level level, shared_ptr<LogEvent> event) override;
		//���´��ļ����ļ��򿪳ɹ�����true
		bool reopen();
	private:
		string m_filename;
		ostream m_filestream;
	};

	//��־��ʽ��
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


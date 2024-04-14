#pragma once
#include "global.h"
namespace SylarSpace
{
	/*
	* ���ϵ��
	* �ȴ���LogEvent��Ϊ��־�¼���
	* Logger�ࣨ�������LogAppender����������LogAppender����
	* LogAppender����LogFormatter������־��ʽ�����LogEvent
	*/
	class LogLevel;			//��־����
	class LogEvent;			//��־�¼�
	class LogAppender;		//��־�����ַ
	class LogFormatter;		//��־��ʽ��



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
		//��Level������ת��Ϊ�ַ���������Ϊ��̬����ʹ���������δ����LogLevel�����ʱ�����ã�
		static const string ToString(const Level level);
	};



	//��־�¼�
	class LogEvent
	{
	public:
		LogEvent(const string& filename,const string& threadname,const int32_t line,const uint32_t elapse,
			const uint32_t thread_id, const uint64_t fiber_id, const uint64_t time);

		//����stringstream
		void setSstream(const string& str);

		//��ȡ˽�б���
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
		const string m_filename;			//�ļ������Դ�·����
		const string m_threadname = 0;		//�߳�����
		int32_t m_line = 0;					//�к�
		uint32_t m_elapse = 0;				//������������ĺ�����
		uint32_t m_thread_id = 0;			//�߳�id
		uint32_t m_fiber_id = 0;			//Э��id
		uint64_t m_time;					//ʱ���
		stringstream m_sstream;				//�¼����ݵ�stringstream
	};



	//��־�����
	class Logger
	{
	public:
		Logger(const string& name = Default_LoggerName);	//����Ĭ����־����Default_LoggerName

		//��־�������
		void log(const LogLevel::Level level,const shared_ptr<const LogEvent> event);

		//��ӻ�ɾ��Appender
		void addAppender(const shared_ptr<LogAppender> appender);	//�ں���appender��formatter�����ã���LogAppender������Ϊconst
		void deleteAppender(const shared_ptr<const LogAppender> appender);

		//��ȡ���޸���־�ȼ�
		LogLevel::Level getlevel()const { return m_level; }
		void setLevel(const LogLevel::Level level) { m_level = level; }

		//��ȡlogger����
		const string& getName()const { return m_name; }

	private:
		string m_name;		//logger����
		LogLevel::Level m_level;	//�������־����������־
		list<shared_ptr<LogAppender>> m_appenders;	//appender����
		shared_ptr<LogFormatter> m_formatter;		//��־��ʽ��
	public:
		//Ĭ����־��ʽģʽ
		const static string Default_FormatPattern;
		//Ĭ����־����
		const static string Default_LoggerName;
		//Ĭ����־ʱ��ģʽ
		const static string Default_DataTimePattern;
	};



	//��־�����ַ
	class LogAppender
	{
	public:
		LogAppender(const LogLevel::Level level = LogLevel::DEBUG, const string& logger_name = Logger::Default_LoggerName);
		virtual ~LogAppender() {}		//�������������Ӧ������Ϊ�麯��
		//��־����������� Logger::log���ã����麯�����������ʵ�֣�
		virtual void log(const string& logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event) = 0;

		//��ȡ���޸�formatter
		shared_ptr<LogFormatter> getFormatter()const { return m_formatter; }
		void setFormatter(const shared_ptr<const LogFormatter> formatter) { m_formatter = formatter; }

		//��ȡ���޸�LogLevel
		LogLevel::Level getLevel()const { return m_level; }
		void setLevel(const LogLevel::Level level) { m_level = level; }

	protected:
		string m_logger_name;		//��־����
		LogLevel::Level m_level;	//��־����
		shared_ptr<LogFormatter> m_formatter;	//��־��ʽ��
	};

	//���������̨��Appender�����м̳���LogAppender��
	class StdoutLogAppender :public LogAppender
	{
	public:
		StdoutLogAppender(const LogLevel::Level level = LogLevel::DEBUG, const string& logger_name= Logger::Default_LoggerName);

		//��־����������� Logger::log���ã���дLogAppender����Ӧ������
		virtual void log(const string& logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event) override;
	};

	//������ļ���Appender�����м̳���LogAppender��
	class FileLogAppender :public LogAppender
	{
	public:
		FileLogAppender(const LogLevel::Level level = LogLevel::DEBUG, const string& logger_name = Logger::Default_LoggerName,const string& filename="./log.txt");	//����Ĭ����־��ַ�ļ���Ϊ"./log.txt"

		//��־����������� Logger::log���ã���дLogAppender����Ӧ������
		virtual void log(const string& logger_name,const LogLevel::Level level, const shared_ptr<const LogEvent> event) override;

		//���´��ļ����ļ��򿪳ɹ�����true
		bool reopen();

	private:
		string m_filename;		//��־��ַ�ļ���
		ofstream m_filestream;	//��־��ַ�ļ���
	};



	//��־��ʽ��
	class LogFormatter
	{
		//��������
		//%m ��Ϣ��
		//%p level
		//%r ������ʱ��
		//%c ��־����
		//%t �߳�id
		//%n �س�����
		//%d ʱ��
		//%f �ļ�������·����
		//%l �к�
		//%T tab
		//%N �߳�����
		// ����ģʽʾ����
		// "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
		// "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
	public:
		LogFormatter(const string& pattern);

		//formatter����������Appender����
		string format(const string& logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event);

	private:

		//������־ģʽ��pattern����ʼ����־��ʽ
		void initialize();

		//������α���������־��const string& formatΪ�������е����ݣ�
		string write_piece(const string& logger_name,const LogLevel::Level level,const shared_ptr<const LogEvent> event,const string& mode="",const string& format="");
	private:
		string m_pattern;	//��־��ģʽ����Logger���ʼ��
		vector<pair<string,string>> m_modes_and_formats;	//�������ĵ�����־ģʽ����,ǰ��Ϊ����'%'�õ���ģʽ������Ϊ'{}'�ڵ�����
	};
}


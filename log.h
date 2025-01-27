#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>

#include <map>
#include <list>
#include <vector>

#include <cassert>

#include "mutex.h"

namespace LogSpace
{
	using namespace MutexSpace;

	using std::string;
	using std::map;
	using std::list;
	using std::vector;
	using std::pair;
	using std::make_pair;

	using std::shared_ptr;

	using std::ofstream;
	using std::stringstream;
	using std::to_string;
	using std::cout;
	using std::endl;

	/*
	* ���ϵ��
	* �ȴ���LogEvent������־�¼����ݣ�
	* Logger�ࣨ�������LogAppender����������LogAppender����
	* LogAppender����LogFormatter������־��ʽ�����LogEvent
	* LoggerManager��װ�ж��Logger�����������ڴ�������ʱ�Դ�Ĭ��Logger������ʹ��
	*/
	/*
	* ��־ϵͳ���÷�����
	* 1.�ȴ���LogEvent������setSstream()�ȷ���������־�¼����ݣ�
	* 2.�ٴ���Logger��LoggerAppender������Ҫʱ�ٴ���LogFormatter����
	* 3.Ȼ�����Logger�����addAppender()��������LogAppender����Logger�����ڲ���
	* �����ʹ��LoggerManager���ڶ�����������ʡ�ԣ�
	* 4.���Logger������LogEvent����Ϊ��������log()���������־
	*/
	class LogLevel;			//��־����
	class LogEvent;			//��־�¼�
	class LogAppender;		//��־�����ַ
	class LogFormatter;		//��־��ʽ��
	class LoggerManager;	//��־�����������



	//��־����
	class LogLevel
	{
	public:
		enum Level
		{
			LOG_DEBUG = 1,
			LOG_INFO = 2,
			LOG_WARN = 3,
			LOG_ERROR = 4,
			LOG_FATAL = 5
		};
	public:
		//��Level������ת��Ϊ�ַ���������Ϊ��̬����ʹ���������δ����LogLevel�����ʱ�����ã�
		static const string toString(const Level level);
	};


	//��־�¼�
	class LogEvent
	{
	public:
		LogEvent(const string& filename, const int32_t line);

		//����stringstream
		void setSstream(const string& str);

		//��ȡ˽�б���
		const string getFilename()const { return m_filename; }
		int32_t getLine()const { return m_line; }
		pid_t getThread_id()const { return m_thread_id; }
		const string getThreadname()const { return m_thread_name; }
		uint32_t getFiber_id()const { return m_fiber_id; }
		uint32_t getElapse()const { return m_elapse; }
		uint64_t getTime()const { return m_time; }
		const string getContent()const { return m_sstream.str(); }
		stringstream& getSstream() { return m_sstream; }
	private:
		const string m_filename;			//�ļ������Դ�·����
		int32_t m_line = 0;					//�к�
		pid_t m_thread_id = 0;				//�߳�id
		string m_thread_name;				//�߳�����
		uint32_t m_fiber_id = 0;			//Э��id
		uint32_t m_elapse = 0;				//������������ĺ�����
		uint64_t m_time;					//ʱ���
		stringstream m_sstream;				//�¼����ݵ�stringstream
	};



	//��־�����
	class Logger
	{
	public:
		Logger(const string& name = Default_LoggerName);	//����Ĭ����־����Default_LoggerName���Լ�Ĭ����־ģʽDefault_FormatPattern

		//��־�������
		void log(const LogLevel::Level level,const shared_ptr<const LogEvent> event);

		//���ӻ�ɾ��Appender
		void addAppender(const shared_ptr<LogAppender> appender);	//�ں���appender��formatter�����ã���LogAppender������Ϊconst
		void deleteAppender(const shared_ptr<const LogAppender> appender);

		//�޸�formatter
		void setFormatter(const shared_ptr<LogFormatter> formatter);	//���ݵ���shared_ptr��ʹ���˻���������LogFormatter��������const
		void setFormatter(const string& str);	

		//��ȡ���޸���־�ȼ�
		LogLevel::Level getlevel();		//ʹ���˻���������getlevel()��������const
		void setLevel(const LogLevel::Level level);

		//��ȡlogger����
		const string& getName()const { return m_name; }
	public:
		//Ĭ����־��ʽģʽ
		const static string Default_FormatPattern;
		//Ĭ����־����
		const static string Default_LoggerName;
		//Ĭ����־ʱ��ģʽ
		const static string Default_DataTimePattern;
	private:
		string m_name;		//logger����
		LogLevel::Level m_level;	//�������־����������־
		list<shared_ptr<LogAppender>> m_appenders;	//appender����
		shared_ptr<LogFormatter> m_formatter;		//��־��ʽ��

		SpinLock m_mutex;		//����������������
	};



	//��־�����ַ
	class LogAppender
	{
	public:
		LogAppender(const LogLevel::Level level = LogLevel::LOG_DEBUG, const string& logger_name = Logger::Default_LoggerName);
		virtual ~LogAppender() {}		//�������������Ӧ������Ϊ�麯��
		//��־����������� Logger::log���ã����麯�����������ʵ�֣�
		virtual void log(const string& logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event) = 0;

		//��ȡ���޸�LogLevel
		LogLevel::Level getLevel();
		void setLevel(const LogLevel::Level level);

		//��ȡformatter
		shared_ptr<LogFormatter> getFormatter();	//ʹ�û��������ʲ���const
		//�޸�formatter
		void setFormatter(const shared_ptr<LogFormatter> formatter);	//���ݵ���shared_ptr���ʲ�������const
	protected:
		string m_logger_name;		//��־����
		LogLevel::Level m_level;	//��־����
		shared_ptr<LogFormatter> m_formatter;	//��־��ʽ��

		SpinLock m_mutex;		//����������������
	};

	//���������̨��Appender�����м̳���LogAppender��
	class StdoutLogAppender :public LogAppender
	{
	public:
		StdoutLogAppender(const LogLevel::Level level = LogLevel::LOG_DEBUG, const string& logger_name= Logger::Default_LoggerName);

		//��־����������� Logger::log���ã���дLogAppender����Ӧ������
		virtual void log(const string& logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event) override;
	};

	//������ļ���Appender�����м̳���LogAppender��
	class FileLogAppender :public LogAppender
	{
	public:
		FileLogAppender(const LogLevel::Level level = LogLevel::LOG_DEBUG, const string& logger_name = Logger::Default_LoggerName,const string& filename="./log.txt");	//����Ĭ����־��ַ�ļ���Ϊ"./log.txt"

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
		//%X Э��id
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
		string write_piece(const string& logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event, const string& mode = "", const string& format = "");
	private:
		string m_pattern;	//��־��ģʽ����Logger���ʼ��
		vector<pair<string,string>> m_modes_and_formats;	//�������ĵ�����־ģʽ����,ǰ��Ϊ����'%'�õ���ģʽ������Ϊ'{}'�ڵ�����
	};



	//��־�����������
	class LoggerManager
	{
	public:
		LoggerManager();	//��ʼ��Ĭ��logger

		//��logger_name��ȡlogger����δ��ѯ���򴴽�֮
		shared_ptr<Logger> getLogger(const string& logger_name);
		//��ȡĬ��logger
		shared_ptr<Logger> getDefault_logger();
	private:
		map<string, shared_ptr<Logger>> m_loggers;	//ʹ��logger_nameΪ�����в��ҵ�logger����
		shared_ptr<Logger> m_default_logger;	//Ĭ��logger

		SpinLock m_mutex;		//����������������
	};
}


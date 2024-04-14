#include "log.h"
namespace SylarSpace
{
	//class LogLevel
	
	//��Level������ת��Ϊ�ַ���������Ϊ��̬����ʹ���������δ����LogLevel�����ʱ�����ã�
	const string LogLevel::ToString(const Level level)
	{
		switch (level)
		{
		case SylarSpace::LogLevel::DEBUG:
			return "DEBUG";
			break;
		case SylarSpace::LogLevel::INFO:
			return "INFO";
			break;
		case SylarSpace::LogLevel::WARN:
			return "WARN";
			break;
		case SylarSpace::LogLevel::ERROR:
			return "ERROR";
			break;
		case SylarSpace::LogLevel::FATAL:
			return "FATAL";
			break;
		default:
			return "UNKNOW";
			break;
		}
	}



	//class LogEvent
	LogEvent::LogEvent(const string& filename, const string& threadname, const int32_t line, const uint32_t elapse,
		const uint32_t thread_id, const uint64_t fiber_id, const uint64_t time)
		:m_filename(filename),m_threadname(threadname), m_line(line), m_elapse(elapse), m_thread_id(thread_id),
		m_fiber_id(fiber_id), m_time(time) {}
	//����stringstream
	void LogEvent::setSstream(const string& str)
	{
		//���stringstream��־
		m_sstream.clear();
		//���stringstream����
		m_sstream.str("");
		//д��������
		m_sstream << str;
	}



	//class Logger:
	Logger::Logger(const string& name) :m_name(name), m_level(LogLevel::DEBUG)
	{
		//����Logger����ʱ�Զ�����formatterΪĬ����־ģʽ
		m_formatter.reset(new LogFormatter(Default_FormatPattern));
	}

	//��־�������
	void Logger::log(const LogLevel::Level level,const shared_ptr<const LogEvent> event)
	{
		//ֻ����־�ȼ����ڻ����Logger�����־�ȼ������
		if (level >= m_level)
		{
			for (auto& appender : m_appenders)
			{
				//����Appender�����log����
				appender->log(m_name, level, event);
			}
		}
	}

	//��ӻ�ɾ��Appender
	void Logger::addAppender(const shared_ptr<LogAppender> appender)
	{
		//�������������Appenderû������Formatter����̳�Logger�����Formatter
		if (!appender->getFormatter())
		{
			appender->setFormatter(m_formatter);
		}
		m_appenders.push_back(appender);
	}
	void Logger::deleteAppender(const shared_ptr<const LogAppender> appender)
	{
		for (auto iterator = m_appenders.begin(); iterator != m_appenders.end(); ++iterator)
		{
			if (*iterator == appender)
			{
				m_appenders.erase(iterator);
				break;
			}
		}
	}

	//Ĭ����־��ʽģʽ
	const string Logger::Default_FormatPattern = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T[%p]%T[%c]%T%f:%l%T%m%n";
	//Ĭ����־����
	const string Logger::Default_LoggerName = "root_logger";
	//Ĭ����־ʱ��ģʽ
	const string Logger::Default_DataTimePattern = "%Y-%m-%d %H:%M:%S";



	//class LogAppender
	LogAppender::LogAppender(const LogLevel::Level level, const string& logger_name) :m_level(level),m_logger_name(logger_name){}


	//class StdoutLogAppender:
	StdoutLogAppender::StdoutLogAppender(const LogLevel::Level level, const string& logger_name) :LogAppender(level, logger_name) {}
	void StdoutLogAppender::log(const string& logger_name,const LogLevel::Level level,const shared_ptr<const LogEvent> event)
	{
		//ֻ����־�ȼ����ڻ����Appender�����־�ȼ������
		if (level >= m_level)
		{
			cout << m_formatter->format(logger_name, level, event);
		}
	}


	//class FileLogAppender:
	FileLogAppender::FileLogAppender(const LogLevel::Level level, const string& logger_name,const string& filename)
		:LogAppender(level,logger_name), m_filename(filename) {}
	void FileLogAppender::log(const string& logger_name,const LogLevel::Level level,const shared_ptr<const LogEvent> event)
	{
		//�������ļ������ж��ļ��Ƿ�ɹ���
		assert(reopen());
		//ֻ����־�ȼ����ڻ����Appender�����־�ȼ������
		if (level >= m_level)
		{
			m_filestream << m_formatter->format(logger_name, level, event);
		}
	}
	bool FileLogAppender::reopen()
	{
		//����ļ��Ѵ򿪣��Ƚ���ر�
		if (m_filestream.is_open())
		{
			m_filestream.close();
		}
		//�ļ���׷��ģʽ�򿪣���ֹ������־�ļ�ԭ�е�����
		m_filestream.open(m_filename, std::ios_base::app);
		//�ļ��򿪳ɹ�����true
		return m_filestream.is_open();
	}



	//class LogFormatter:
	LogFormatter::LogFormatter(const string& pattern) :m_pattern(pattern)
	{
		//����LogFormatter���������������־ģʽ��pattern����ʼ����־��ʽ
		initialize();
	}
	
	//formatter����������Appender����
	string LogFormatter::format(const string& logger_name,const LogLevel::Level level,const shared_ptr<const LogEvent> event)
	{
		string str;
		//��������ϵ���־ģʽд��
		for (auto x : m_modes_and_formats)	//�������ĵ�����־ģʽ����,ǰ��Ϊ����'%'�õ���ģʽ������Ϊ'{}'�ڵ�����
		{
			str += write_piece(logger_name, level, event, x.first, x.second);
		}
		return str;
	}

	//������־ģʽ��pattern����ʼ����־��ʽ
	void LogFormatter::initialize()
	{
		//��ʾ��ǰ�Ľ����׶�
		enum Status
		{
			//�ȴ�'{'
			WAIT_FOR_OPEN_BRACE = 1,
			//�ȴ�'}'
			WAIT_FOR_CLOSE_BRACE = 2,
			//�ȴ��ѽ���
			WAIT_FOR_NOTHING = 3
		};

		//�����ַ���
		string normal_str;
		//�������pattern�ڵ��ַ�
		for (int i = 0; i < m_pattern.size(); ++i)
		{
			//�������%����볣���ַ���
			if (m_pattern[i] != '%')
			{
				normal_str.push_back(m_pattern[i]);
			}
			else if (i + 1 < m_pattern.size())
			{
				//�����������%��˵�������%�����볣���ַ���
				if (m_pattern[i + 1] == '%')
				{
					++i;
					normal_str.push_back(m_pattern[i]);
				}
				//����ʼ����
				else
				{
					//����֮ǰ�Ȱ��Ѷ��ĳ����ַ���д��m_modes_and_formats
					if (!normal_str.empty())
					{
						m_modes_and_formats.push_back(make_pair(normal_str, ""));
					}
					//д�����ճ����ַ����������ظ�
					normal_str.clear();

					//���ó�ʼ�����׶�
					Status format_status = WAIT_FOR_OPEN_BRACE;
					int str_inbrace_begin = 0, mode_begin = i + 1;	//str_inbrace_begin������mode_begin��'%'����һ��λ�ÿ�ʼ
					string mode("%"), str_inbrace;

					//�������'%'������ַ�
					for (; i + 1 < m_pattern.size();++i)
					{
						//�����δ����'{'
						if (format_status == WAIT_FOR_OPEN_BRACE)
						{
							//�����i + 1���ַ�����ĸ������mode
							if (isalpha(m_pattern[i + 1]))
							{
								mode.push_back(m_pattern[i + 1]);
							}
							//�����i + 1���ַ���'{'
							else if (m_pattern[i + 1] == '{')
							{
								//�л�״̬
								format_status = WAIT_FOR_CLOSE_BRACE;
								str_inbrace_begin = i + 2;		//str_inbrace_begin��Ϊ'{'����һ��λ�ã���i+2
							}
							//����ֱ�ӽ���ѭ��
							else
							{
								break;
							}
						}
						//���������'{',����δ����'}'������'}'
						else if (format_status == WAIT_FOR_CLOSE_BRACE && m_pattern[i + 1] == '}')
						{
							//����'}'�л�״̬������ѭ��
							format_status = WAIT_FOR_NOTHING;
							//��'{}'�ڵ��ַ���д��str_inbrace
							str_inbrace = m_pattern.substr(str_inbrace_begin, i + 1 - str_inbrace_begin);	
							break;
						}
					}

					//�����δ������{
					if (format_status == WAIT_FOR_OPEN_BRACE)
					{
						if (!mode.empty())
						{
							//˵��û��'{}'��ֱ�Ӱ�mode�Ϳ��ַ���д����־ģʽ����
							m_modes_and_formats.push_back(make_pair(mode, ""));
						}
					}
					//��״̬1�ͽ����˽�����˵��ȱʧ��'}'������
					else if (format_status == WAIT_FOR_CLOSE_BRACE)
					{
						cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << endl;
						//��������ʾд����־ģʽ����
						m_modes_and_formats.push_back(make_pair("<<pattern_error>>", ""));
					}
					//��������{}���ݵĶ�ȡ
					else if (format_status == WAIT_FOR_NOTHING)
					{
						if (!mode.empty())
						{
							//��mode��str_inbraceһ��д����־ģʽ����
							m_modes_and_formats.push_back(make_pair(mode, str_inbrace));
						}
					}
				}
			}
		}
		//����������µĳ����ַ���������д����־ģʽ����
		if (!normal_str.empty())
		{
			m_modes_and_formats.push_back(make_pair(normal_str, ""));
		}
	}

	//������α���������־
	string LogFormatter::write_piece(const string& logger_name,const LogLevel::Level level,const shared_ptr<const LogEvent> event, const string& mode, const string& format)
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
		
		if (mode == "%m")
		{
			//%m ��Ϣ��
			return event->getContent();
		}
		else if (mode == "%p")
		{
			//%p level
			return LogLevel::ToString(level);	//��LogLevel::Level����ת��Ϊstring����
		}
		else if (mode == "%r")
		{
			//%r ������ʱ��
			return to_string(event->getElapse());
		}
		else if (mode == "%c")
		{
			//%c ��־����
			return logger_name;
		}
		else if (mode == "%t")
		{
			//%t �߳�id
			return to_string(event->getThread_id());
		}
		else if (mode == "%n")
		{
			//%n �س�����
			return "\n";
		}
		else if (mode == "%d")
		{
			//%d ʱ��
			struct tm tm;
			time_t time = event->getTime();
			//��Unix��Ԫʱ��ת��Ϊ����ʱ�䣬������ʱ��������ʱ������
			localtime_r(&time, &tm);
			char buffer[64];
			//���û��'{}'�ڵĸ�ʽ������Ĭ����־ʱ���ʽ
			if (format.empty())
			{
				strftime(buffer, sizeof(buffer), Logger::Default_DataTimePattern.c_str(), &tm);
			}
			else
			{
				strftime(buffer, sizeof(buffer), format.c_str(), &tm);
			}
			string str = buffer;
			return str;
		}
		else if (mode == "%f")
		{
			//%f �ļ���
			return event->getFilename();
		}
		else if (mode == "%l")
		{
			//%l �к�
			return to_string(event->getLine());
		}
		else if (mode == "%T")
		{
			//%T tab
			return "\t";
		}
		else if (mode == "%N")
		{
			//%N �߳�����
			return event->getThreadname();
		}
		//���ʶ��ʧ�ܣ����������ַ�������
		else
		{
			return mode;
		}
	}
}
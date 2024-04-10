#include "log.h"
namespace SylarSpace
{
	//class LogLevel
	const string LogLevel::ToString(Level level)
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

	//class Logger:
	Logger::Logger(const string& name):m_name(name){}
	//日志输出函数
	void Logger::log(LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		//只有日志等级大于Logger类的日志等级才输出
		if (level >= m_level)
		{
			for (auto& appender : m_appenders)
			{
				appender->log(level, event);
			}
		}
	}

	//各个日志级别对应的日志输出函数
	void Logger::debug(shared_ptr<LogEvent> event)
	{
		log(LogLevel::DEBUG, event);
	}
	void Logger::info(shared_ptr<LogEvent> event)
	{
		log(LogLevel::INFO, event);
	}
	void Logger::warn(shared_ptr<LogEvent> event)
	{
		log(LogLevel::WARN, event);
	}
	void Logger::error(shared_ptr<LogEvent> event)
	{
		log(LogLevel::ERROR, event);
	}
	void Logger::fatal(shared_ptr<LogEvent> event)
	{
		log(LogLevel::FATAL, event);
	}

	//添加或删除Appender
	void Logger::addAppender(shared_ptr<LogAppender> appender)
	{
		m_appenders.push_back(appender);
	}
	void Logger::deleteAppender(shared_ptr<LogAppender> appender)
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


	//class StdoutLogAppender:
	void StdoutLogAppender::log(shared_ptr<Logger> logger,LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		if (level > m_level)
		{
			cout << m_formatter->format(event);
		}
	}


	//class FileLogAppender:
	FileLogAppender::FileLogAppender(const string& filename) :m_filename(filename){}
	void FileLogAppender::log(shared_ptr<Logger> logger,LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		if (level > m_level)
		{
			m_filestream << m_formatter->format(event);
		}
	}
	bool FileLogAppender::reopen()
	{
		if (m_filestream)
		{
			m_filestream.close();
		}
		m_filestream.open(m_filename);
		return m_filestream.is_open();
	}

	//class LogFormatter:
	LogFormatter::LogFormatter(const string& pattern)
	{

	}
	//%t	%thread_id%m%n
	string LogFormatter::format(shared_ptr<Logger> logger,LogLevel::Level level,shared_ptr<LogEvent> event)
	{
		stringstream ss;
		for (auto& i : m_items)
		{
			i->format(logger, ss, level ,event);
		}
		return ss.str();
	}

	void LogFormatter::initialize()
	{
		//string,format,type
		vector<tuple<string,string, int>> vec;
		string normal_str;
		for (size_t i = 0; i < m_pattern.size(); ++i)
		{
			if (m_pattern[i] != '%')
			{
				normal_str.append(1,m_pattern[i]);
			}
			else if (i+1<m_pattern.size())
			{
				if (m_pattern[i + 1] == '%')
				{
					normal_str.append(1, '%');
				}
			}
			else
			{
				size_t n = i + 1, format_begin = 0;
				int format_status = 0;
				string str, fmt;
				while (n < m_pattern.size())
				{
					if (isspace(m_pattern[n]))
					{
						break;
					}
					if (format_status == 0)
					{
						if (m_pattern[n] == '{')
						{
							str = m_pattern.substr(i + 1, n - i);
							format_status = 1;		//解析格式
							++n;
							format_begin = n;
							continue;
						}
					}
					if (format_status == 1)
					{
						if (m_pattern[n] == '}')
						{
							fmt = m_pattern.substr(format_begin + 1, n - format_begin);
							format_status = 2;
							break;
						}
					}
				}
				if (format_status == 0)
				{
					if (!normal_str.empty())
					{
						vec.push_back(make_tuple(normal_str, "", 0));
					}
					str = m_pattern.substr(i + 1, n - i - 1);
					vec.push_back(make_tuple(str, fmt, 1));
					i = n;
				}
				else if (format_status == 1)
				{
					cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << endl;
					vec.push_back(make_tuple("<<pattern_error>>", fmt, 0));
				}
				else if (format_status == 2)
				{
					if (!normal_str.empty())
					{
						vec.push_back(make_tuple(normal_str, "", 0));
					}
					vec.push_back(make_tuple(str, fmt, 1));
					i = n;
				}
			}
			if (!normal_str.empty())
			{
				vec.push_back(make_tuple(normal_str, "", 0));
			}

			static map<string, function<shared_ptr<FormatItem>(const string& str)>> s_format_items =
			{
				{"m",[](const string& fmt) { return shared_ptr<FormatItem>(new  MessageFormatItem(fmt)); } }
			};

			for (auto& i : vec)
			{
				if (std::get<2>(i) == 0)
				{
					m_items.push_back(shared_ptr<FormatItem>(new  StringFormatItem(std::get<0>(i))));
				}
				else
				{
					auto it = s_format_items.find(std::get<0>(i));
					if (it == s_format_items.end())
					{
						m_items.push_back(shared_ptr<FormatItem>(new  StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
					}
					else
					{
						m_items.push_back(it->second(std::get<1>(i)));
					}
				}
				cout << std::get<0>(i) << " - " << std::get<1>(i) << " - " << std::get<2>(i) << endl;
			}

			//%m 消息体
			//%p level
			//%r 启动后时间
			//%c 日志名称
			//%t 线程id
			//%n 回车换行
			//%d 时间
			//%f 文件名
			//%i 行号
		}
	}

	void LogFormatter::MessageFormatItem::format(shared_ptr<Logger> logger,ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		os << event->getContent();
	}
	void LogFormatter::LevelFormatItem::format(shared_ptr<Logger> logger,ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		os << LogLevel::ToString(level);
	}
	void LogFormatter::ElapseFormatItem::format(shared_ptr<Logger> logger,ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		os << event->getElapse();
	}
	void LogFormatter::FilenameFormatItem::format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		os << event->getFilename();
	}
	void LogFormatter::Thread_idFormatItem::format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		os << event->getThread_id();
	}
	void LogFormatter::Fiber_idFormatItem::format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		os << event->getFiber_id();
	}
	void LogFormatter::DataTimeFormatItem::format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		os << event->getTime();
	}
	LogFormatter::DataTimeFormatItem::DataTimeFormatItem(const string& format):m_format(format){}
	void LogFormatter::LineFormatItem::format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		os << event->getLine();
	}
	void LogFormatter::EnterFormatItem::format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		os << endl;
	}
	void LogFormatter::StringFormatItem::format(shared_ptr<Logger> logger, ostream& os, LogLevel::Level level, shared_ptr<LogEvent> event)
	{
		os << m_string;
	}
	LogFormatter::StringFormatItem::StringFormatItem(const string& str) :m_string(str) {}
}
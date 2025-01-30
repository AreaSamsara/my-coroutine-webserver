#include "common/log.h"
#include "common/utility.h"
#include "concurrent/fiber.h"

namespace LogSpace
{
	using namespace UtilitySpace;

	// class LogLevel:static public
	// 将Level共用体转化为字符串（声明为静态方法使得其可以在未创建LogLevel类对象时被调用）
	const string LogLevel::toString(const Level level)
	{
		switch (level)
		{
		case LogLevel::LOG_DEBUG:
			return "DEBUG";
			break;
		case LogLevel::LOG_INFO:
			return "INFO";
			break;
		case LogLevel::LOG_WARN:
			return "WARN";
			break;
		case LogLevel::LOG_ERROR:
			return "ERROR";
			break;
		case LogLevel::LOG_FATAL:
			return "FATAL";
			break;
		default:
			return "UNKNOW";
			break;
		}
	}

	// class LogEvent:public
	LogEvent::LogEvent(const string &filename, const int32_t line)
		: m_filename(filename), m_line(line), m_thread_id(GetThread_id()), m_thread_name(GetThread_name()),
		  m_fiber_id(GetFiber_id()), m_elapse(0), m_time(time(0)) {}

	// 设置stringstream
	void LogEvent::setSstream(const string &str)
	{
		// 清空stringstream标志
		m_sstream.clear();
		// 清空stringstream内容
		m_sstream.str("");
		// 写入新内容
		m_sstream << str;
	}

	// class Logger:public
	Logger::Logger(const string &name) : m_name(name), m_level(LogLevel::LOG_DEBUG)
	{
		// 构造Logger对象时自动设置formatter为默认日志模式
		m_formatter.reset(new LogFormatter(Default_FormatPattern));
	}

	// 日志输出函数
	void Logger::log(const LogLevel::Level level, const shared_ptr<const LogEvent> event)
	{
		// 只有日志等级大于或等于Logger类的日志等级才输出
		if (level >= m_level)
		{
			// 先监视互斥锁，保护m_appenders和m_name
			ScopedLock<SpinLock> lock(m_mutex);
			for (auto &appender : m_appenders)
			{
				// 调用Appender对象的log方法
				appender->log(m_name, level, event);
			}
		}
	}

	// 添加或删除Appender
	void Logger::addAppender(const shared_ptr<LogAppender> appender)
	{
		// 先监视互斥锁，保护m_appenders和m_formatter
		ScopedLock<SpinLock> lock(m_mutex);
		// 如果即将新增的Appender没有设置Formatter，则继承Logger对象的Formatter
		if (!appender->getFormatter())
		{
			// 先监视互斥锁
			// MutexType mutex = appender->getMutex();
			// ScopedLock<MutexType> appender_lock(mutex);
			appender->setFormatter(m_formatter);
		}
		m_appenders.push_back(appender);
	}
	void Logger::deleteAppender(const shared_ptr<const LogAppender> appender)
	{
		// 先监视互斥锁，保护m_appenders
		ScopedLock<SpinLock> lock(m_mutex);
		for (auto iterator = m_appenders.begin(); iterator != m_appenders.end(); ++iterator)
		{
			if (*iterator == appender)
			{
				m_appenders.erase(iterator);
				break;
			}
		}
	}

	// 修改formatter
	void Logger::setFormatter(const shared_ptr<LogFormatter> formatter)
	{
		// 先监视互斥锁，保护m_formatter
		ScopedLock<SpinLock> lock(m_mutex);
		m_formatter = formatter;

		// 将所有包含的Appender的Formatter一并修改
		for (auto &appender : m_appenders)
		{
			// 先监视互斥锁
			// MutexType mutex = appender->getMutex();
			// ScopedLock<MutexType> appender_lock(mutex);
			appender->setFormatter(formatter);
		}
	}
	void Logger::setFormatter(const string &str)
	{
		// 用string参数创建formatter对象，再调用另一个重载
		shared_ptr<LogFormatter> formatter(new LogFormatter(str));
		setFormatter(formatter);
	}

	// 读取或修改日志等级
	LogLevel::Level Logger::getlevel()
	{
		// 先监视互斥锁，保护m_level
		ScopedLock<SpinLock> lock(m_mutex);
		return m_level;
	}
	void Logger::setLevel(const LogLevel::Level level)
	{
		// 先监视互斥锁，保护m_level
		ScopedLock<SpinLock> lock(m_mutex);
		m_level = level;
	}

	// class Logger:static public variable
	// 默认日志格式模式
	const string Logger::Default_FormatPattern = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%X%T[%p]%T[%c]%T%f:%l%T%m%n";
	// 默认日志名称
	const string Logger::Default_LoggerName = "root_logger";
	// 默认日志时间模式
	const string Logger::Default_DataTimePattern = "%Y-%m-%d %H:%M:%S";

	// class LogAppender:public
	LogAppender::LogAppender(const LogLevel::Level level, const string &logger_name) : m_level(level), m_logger_name(logger_name) {}
	// 读取或修改LogLevel
	LogLevel::Level LogAppender::getLevel()
	{
		// 先监视互斥锁，保护m_level
		ScopedLock<SpinLock> lock(m_mutex);
		return m_level;
	}
	void LogAppender::setLevel(const LogLevel::Level level)
	{
		// 先监视互斥锁，保护m_level
		ScopedLock<SpinLock> lock(m_mutex);
		m_level = level;
	}

	// 读取formatter
	shared_ptr<LogFormatter> LogAppender::getFormatter()
	{
		// 先监视互斥锁，保护m_formatter
		ScopedLock<SpinLock> lock(m_mutex);
		return m_formatter;
	}
	// 修改formatter
	void LogAppender::setFormatter(const shared_ptr<LogFormatter> formatter)
	{
		// 先监视互斥锁，保护m_formatter
		ScopedLock<SpinLock> lock(m_mutex);
		m_formatter = formatter;
	}

	// class StdoutLogAppender:public
	StdoutLogAppender::StdoutLogAppender(const LogLevel::Level level, const string &logger_name) : LogAppender(level, logger_name) {}
	// 日志输出函数，由 Logger::log调用（重写LogAppender的相应方法）
	void StdoutLogAppender::log(const string &logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event)
	{
		// 只有日志等级大于或等于Appender类的日志等级才输出
		if (level >= m_level)
		{
			// 先监视互斥锁，保护m_formatter和cout
			ScopedLock<SpinLock> lock(m_mutex);
			cout << m_formatter->format(logger_name, level, event);
		}
	}

	// class FileLogAppender:public
	FileLogAppender::FileLogAppender(const LogLevel::Level level, const string &logger_name, const string &filename)
		: LogAppender(level, logger_name), m_filename(filename) {}
	// 日志输出函数，由 Logger::log调用（重写LogAppender的相应方法）
	void FileLogAppender::log(const string &logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event)
	{
		// 先重启文件，并判断文件是否成功打开
		assert(reopen());
		// 只有日志等级大于或等于Appender类的日志等级才输出
		if (level >= m_level)
		{
			// 先监视互斥锁，保护m_formatter和m_filestream
			ScopedLock<SpinLock> lock(m_mutex);
			m_filestream << m_formatter->format(logger_name, level, event);
		}
	}
	// 重新打开文件，文件打开成功返回true
	bool FileLogAppender::reopen()
	{
		// 如果文件已打开，先将其关闭
		if (m_filestream.is_open())
		{
			m_filestream.close();
		}
		// 文件以追加模式打开，防止覆盖日志文件原有的内容
		m_filestream.open(m_filename, std::ios_base::app);
		// 文件打开成功返回true
		return m_filestream.is_open();
	}

	// class LogFormatter:public
	LogFormatter::LogFormatter(const string &pattern) : m_pattern(pattern)
	{
		// 构造LogFormatter对象后立即按照日志模式（pattern）初始化日志格式
		initialize();
	}

	// formatter主函数，由Appender调用
	string LogFormatter::format(const string &logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event)
	{
		string str;
		// 将解析完毕的日志模式写入
		for (auto &x : m_modes_and_formats) // 被解析的单段日志模式集合,前者为解析'%'得到的模式，后者为'{}'内的内容
		{
			str += write_piece(logger_name, level, event, x.first, x.second);
		}
		return str;
	}

	// class LogFormatter:private
	// 按照日志模式（pattern）初始化日志格式
	void LogFormatter::initialize()
	{
		// 表示当前的解析阶段
		enum Status
		{
			// 等待'{'
			WAIT_FOR_OPEN_BRACE = 1,
			// 等待'}'
			WAIT_FOR_CLOSE_BRACE = 2,
			// 等待已结束
			WAIT_FOR_NOTHING = 3
		};

		// 常规字符串
		string normal_str;
		// 逐个分析pattern内的字符
		for (int i = 0; i < m_pattern.size(); ++i)
		{
			// 如果不是%则加入常规字符串
			if (m_pattern[i] != '%')
			{
				normal_str.push_back(m_pattern[i]);
			}
			else if (i + 1 < m_pattern.size())
			{
				// 如果连续两个%，说明真的是%，加入常规字符串
				if (m_pattern[i + 1] == '%')
				{
					++i;
					normal_str.push_back(m_pattern[i]);
				}
				// 否则开始解析
				else
				{
					// 解析之前先把已读的常规字符串写入m_modes_and_formats
					if (!normal_str.empty())
					{
						m_modes_and_formats.push_back(make_pair(normal_str, ""));
					}
					// 写入后清空常规字符串，避免重复
					normal_str.clear();

					// 设置初始解析阶段
					Status format_status = WAIT_FOR_OPEN_BRACE;
					int str_in_brace_begin = 0, mode_begin = i + 1; // str_inbrace_begin待定，mode_begin从'%'的下一个位置开始
					string mode("%"), str_in_brace;

					// 逐个解析'%'后面的字符
					for (; i + 1 < m_pattern.size(); ++i)
					{
						// 如果尚未遇到'{'
						if (format_status == WAIT_FOR_OPEN_BRACE)
						{
							// 如果第i + 1个字符是字母，设置mode
							if (isalpha(m_pattern[i + 1]))
							{
								mode.push_back(m_pattern[i + 1]);
							}
							// 如果第i + 1个字符是'{'
							else if (m_pattern[i + 1] == '{')
							{
								// 切换状态
								format_status = WAIT_FOR_CLOSE_BRACE;
								str_in_brace_begin = i + 2; // str_inbrace_begin定为'{'的下一个位置，即i+2
							}
							// 否则直接结束循环
							else
							{
								break;
							}
						}
						// 如果遇到了'{',但尚未遇到'}'且遇到'}'
						else if (format_status == WAIT_FOR_CLOSE_BRACE && m_pattern[i + 1] == '}')
						{
							// 遇到'}'切换状态并结束循环
							format_status = WAIT_FOR_NOTHING;
							// 把'{}'内的字符串写入str_inbrace
							str_in_brace = m_pattern.substr(str_in_brace_begin, i + 1 - str_in_brace_begin);
							// 移动一位防止'}'也被写入
							++i;
							break;
						}
					}

					// 如果从未遇到过{
					if (format_status == WAIT_FOR_OPEN_BRACE)
					{
						if (!mode.empty())
						{
							// 说明没有'{}'，直接把mode和空字符串写入日志模式集合
							m_modes_and_formats.push_back(make_pair(mode, ""));
						}
					}
					// 在状态1就结束了解析，说明缺失了'}'，报错
					else if (format_status == WAIT_FOR_CLOSE_BRACE)
					{
						cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << endl;
						// 将错误提示写入日志模式集合
						m_modes_and_formats.push_back(make_pair("<<pattern_error>>", ""));
					}
					// 如果完成了{}内容的读取
					else if (format_status == WAIT_FOR_NOTHING)
					{
						if (!mode.empty())
						{
							// 把mode和str_inbrace一起写入日志模式集合
							m_modes_and_formats.push_back(make_pair(mode, str_in_brace));
						}
					}
				}
			}
		}
		// 如果还有余下的常规字符串，将其写入日志模式集合
		if (!normal_str.empty())
		{
			m_modes_and_formats.push_back(make_pair(normal_str, ""));
		}
	}

	// 输出单段被解析的日志
	string LogFormatter::write_piece(const string &logger_name, const LogLevel::Level level, const shared_ptr<const LogEvent> event, const string &mode, const string &format)
	{
		// 解析规则：
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
		//%X 协程id
		//  常用模式示例：
		//  "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
		//  "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"

		if (mode == "%m")
		{
			//%m 消息体
			return event->getContent();
		}
		else if (mode == "%p")
		{
			//%p level
			return LogLevel::toString(level); // 将LogLevel::Level类型转换为string类型
		}
		else if (mode == "%r")
		{
			//%r 启动后时间
			return to_string(event->getElapse());
		}
		else if (mode == "%c")
		{
			//%c 日志名称
			return logger_name;
		}
		else if (mode == "%t")
		{
			//%t 线程id
			return to_string(event->getThread_id());
		}
		else if (mode == "%n")
		{
			//%n 回车换行
			return "\n";
		}
		else if (mode == "%d")
		{
			//%d 时间
			struct tm tm;
			time_t time = event->getTime();
			// 将Unix纪元时间转换为本地时间，考虑了时区和夏令时等因素
			localtime_r(&time, &tm);
			char buffer[64];
			// 如果没有'{}'内的格式，赋予默认日志时间格式
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
			//%f 文件名
			return event->getFilename();
		}
		else if (mode == "%l")
		{
			//%l 行号
			return to_string(event->getLine());
		}
		else if (mode == "%T")
		{
			//%T tab
			return "\t";
		}
		else if (mode == "%N")
		{
			//%N 线程名称
			return event->getThreadname();
		}
		else if (mode == "%X")
		{
			//%X 协程id
			return to_string(event->getFiber_id());
		}
		// 如果识别失败，当作常规字符串处理
		else
		{
			return mode;
		}
	}

	// class LogManager:public
	LoggerManager::LoggerManager()
	{
		// 初始化默认logger
		m_default_logger.reset(new Logger);

		// 添加默认StdoutLogAppender
		m_default_logger->addAppender(shared_ptr<LogAppender>(new StdoutLogAppender));

		// 添加默认FileLogAppender
		m_default_logger->addAppender(shared_ptr<LogAppender>(new FileLogAppender));
	}

	// 按logger_name获取logger
	shared_ptr<Logger> LoggerManager::getLogger(const string &logger_name)
	{
		// 先监视互斥锁，保护m_loggers
		ScopedLock<SpinLock> lock(m_mutex);
		// 如果查找到了相应logger则返回
		auto iterator = m_loggers.find(logger_name);
		if (iterator != m_loggers.end())
		{
			return iterator->second;
		}

		// 否则创建对应logger并返回
		shared_ptr<Logger> logger(new Logger(logger_name));
		m_loggers[logger_name] = logger;
		return logger;
	}

	// 获取默认logger
	shared_ptr<Logger> LoggerManager::getDefault_logger()
	{
		// 先监视互斥锁，保护m_default_logger
		ScopedLock<SpinLock> lock(m_mutex);
		return m_default_logger;
	}
}
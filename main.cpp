#include "log.h"
#include "singleton.h"

int main(int argc, char** argv)
{
	using namespace SingletonSpace;
	using namespace LogSpace;

	//创建logger
	shared_ptr<Logger> logger(new Logger);
	//添加StdoutLogAppender
	logger->addAppender(shared_ptr<LogAppender>(new StdoutLogAppender));

	//添加FileLogAppender
	shared_ptr<FileLogAppender> file_appender(new FileLogAppender(LogLevel::DEBUG,logger->getName(), "./log.txt"));
	//设置FileLogAppender专属的日志模式
	shared_ptr<LogFormatter> fmt(new LogFormatter("%d%T%p%T%m%n"));
	file_appender->setFormatter(fmt);
	file_appender->setLevel(LogLevel::ERROR);
	logger->addAppender(file_appender);

	//设置日志事件
	//__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
	shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));

	event->getSstream() << "test macro info";
	logger->log(LogLevel::INFO, event);

	event->getSstream() << "test macro error";
	logger->log(LogLevel::ERROR, event);

	return 0;
}
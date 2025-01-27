//#include "log.h"
//#include "singleton.h"
//
//int main(int argc, char** argv)
//{
//	using namespace SingletonSpace;
//	using namespace LogSpace;
//
//	//����logger
//	shared_ptr<Logger> logger(new Logger);
//	//����StdoutLogAppender
//	logger->addAppender(shared_ptr<LogAppender>(new StdoutLogAppender));
//
//	//����FileLogAppender
//	shared_ptr<FileLogAppender> file_appender(new FileLogAppender(LogLevel::DEBUG, logger->getName(), "./log.txt"));
//	//����FileLogAppenderר������־ģʽ
//	shared_ptr<LogFormatter> fmt(new LogFormatter("%d%T%p%T%m%n"));
//	file_appender->setFormatter(fmt);
//	file_appender->setLevel(LogLevel::ERROR);
//	logger->addAppender(file_appender);
//
//	//������־�¼�
//	//__FILE__���ص�ǰ�ļ����ļ������Դ�·������__LINE__���ص�ǰ��������;elapseΪ����ֵ
//	shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
//
//	event->getSstream() << "test macro info";
//	logger->log(LogLevel::INFO, event);
//
//	event->getSstream() << "test macro error";
//	logger->log(LogLevel::ERROR, event);
//
//	return 0;
//}
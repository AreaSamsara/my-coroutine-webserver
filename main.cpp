#include "global.h"
#include "log.h"
#include "singleton.h"

int main(int argc, char** argv)
{
	using namespace SylarSpace;

	//����logger
	shared_ptr<Logger> logger(new Logger);
	//���StdoutLogAppender
	logger->addAppender(shared_ptr<LogAppender>(new StdoutLogAppender));

	//���FileLogAppender
	shared_ptr<FileLogAppender> file_appender(new FileLogAppender(LogLevel::DEBUG,logger->getName(), "./log.txt"));
	//����FileLogAppenderר������־ģʽ
	shared_ptr<LogFormatter> fmt(new LogFormatter("%d%T%p%T%m%n"));
	file_appender->setFormatter(fmt);
	file_appender->setLevel(LogLevel::ERROR);
	logger->addAppender(file_appender);

	//������ȡ�߳�id
	stringstream ss;
	ss << std::this_thread::get_id();
	int32_t thread_id = atoi(ss.str().c_str());

	//������־�¼�
	//__FILE__���ص�ǰ�ļ����ļ������Դ�·������__LINE__���ص�ǰ��������
	//elapse��Fiber_idΪ����ֵ
	shared_ptr<LogEvent> event(new LogEvent(__FILE__, "main_thread", __LINE__, 0, thread_id, 2, time(0)));

	event->setSstream("test macro info");
	logger->log(LogLevel::INFO, event);

	event->setSstream("test macro error");
	logger->log(LogLevel::ERROR, event);

	//auto temp_logger = Singleton<Logger>::GetInstance();

	return 0;
}
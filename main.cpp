#include "log.h"
#include "singleton.h"
#include "config.h"
#include "utility.h"

int main(int argc, char** argv)
{
	using namespace SingletonSpace;
	using namespace LogSpace;
	using namespace ConfigSpace;
	
	shared_ptr<ConfigVar<int>> g_int_value_config = Config::Lookup("system.port", 8080, "system.port");
	shared_ptr<ConfigVar<float>> g_float_value_config = Config::Lookup("system.port", 10.2f, "system.port");

	//设置日志事件
	//__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
	shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
	event->getSstream() << g_int_value_config->getValue();
	//使用LoggerManager单例的默认logger输出日志
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
	event->setSstream(g_float_value_config->toString());
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

	return 0;
}
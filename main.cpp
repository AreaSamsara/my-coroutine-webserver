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

	//������־�¼�
	//__FILE__���ص�ǰ�ļ����ļ������Դ�·������__LINE__���ص�ǰ��������;elapseΪ����ֵ
	shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
	event->getSstream() << g_int_value_config->getValue();
	//ʹ��LoggerManager������Ĭ��logger�����־
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
	event->setSstream(g_float_value_config->toString());
	Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

	return 0;
}
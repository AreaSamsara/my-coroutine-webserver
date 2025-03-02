// #include "tcp-ip/address.h"
// #include "common/log.h"
// #include "common/singleton.h"
// #include "common/utility.h"
//
// using namespace AddressSpace;
// using namespace LogSpace;
// using namespace SingletonSpace;
// using namespace UtilitySpace;
//
// void test()
//{
//	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//	log_event->getSstream() << "test begin";
//	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//
//	vector<shared_ptr<Address>> address;
//	bool return_value = Address::Lookup(address, "www.baidu.com");
//	if (!return_value)
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << "Lookup fail";
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
//		return;
//	}
//
//	for (int i = 0; i < address.size(); ++i)
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << i << "-" << address[i]->toString();
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//	}
// }
//
// void test_iface()
//{
//	multimap<string, pair<shared_ptr<Address>, uint32_t>> results;
//	bool return_value = Address::GetInterfaceAddresses(results);
//	if (!return_value)
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << "GetInterfaceAddresses fail";
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
//		return;
//	}
//
//	for (auto& result : results)
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << result.first << "-" << result.second.first->toString() << "-" << result.second.second;
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//	}
// }
//
// void test_ipv4()
//{
//	//auto address = IPAddress::Create("www.baidu.com");
//	auto address = IPAddress::Create("127.0.0.1");
//	if (address)
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << address->toString();
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//	}
// }
//
// int main(int argc, char** argv)
//{
//	test_ipv4();
//	//test();
//	//test_iface();
//	return 0;
// }
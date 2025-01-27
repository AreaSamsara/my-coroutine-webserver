//#include "tcp_server.h"
//#include "log.h"
//#include "singleton.h"
//#include "utility.h"
//
//using namespace LogSpace;
//using namespace SingletonSpace;
//using namespace UtilitySpace;
//using namespace TcpServerSpace;
//
//
//void test()
//{
//	auto address = Address::LookupAny("0.0.0.0:8033");
//	auto unix_addres = shared_ptr<UnixAddress>(new UnixAddress("/tmp/unix_address"));
//
//	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//	log_event->getSstream() << "address: " << address->toString();
//	log_event->getSstream() << "\tunix_address: " << unix_addres->toString();
//	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//
//	vector<shared_ptr<Address>> addresses;
//	vector<shared_ptr<Address>> addresses_fail;
//	addresses.push_back(address);
//	addresses.push_back(unix_addres);
//
//	shared_ptr<TcpServer> tcp_server(new TcpServer);
//	while (!tcp_server->bind(addresses, addresses_fail))
//	{
//		sleep(2);
//	}
//	tcp_server->start();
//}
//
//
//int main(int argc, char** argv)
//{
//	IOManager iomanager(2);
//	iomanager.schedule(test);
//	return 0;
//}
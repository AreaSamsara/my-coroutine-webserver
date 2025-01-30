// #include "http_connection.h"
// #include "common/log.h"
// #include "common/singleton.h"
// #include "common/utility.h"
// #include "servlet.h"
// #include "iomanager.h"
//
// using namespace LogSpace;
// using namespace SingletonSpace;
// using namespace UtilitySpace;
// using namespace HttpConnectionSpace;
// using namespace ByteArraySpace;
// using namespace HttpSpace;
// using namespace IOManagerSpace;
//
// void test_pool()
//{
//	shared_ptr<HttpConnectionPool> connection_pool(new HttpConnectionPool("www.baidu.com", "", 80, 10, 1000 * 30, 5));
//
//	shared_ptr<Timer> timer(new Timer(1000, [connection_pool]()
//		{
//			{
//				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//				log_event->getSstream() << "timer here";
//				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//			}
//
//			auto result = connection_pool->doGet("/", 300);
//			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//			log_event->getSstream() << "result:\n" << result->toString();
//			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//		}
//	, true));
//	IOManager::GetThis()->addTimer(timer);
// }
//
// void run()
//{
//	shared_ptr<Address> address = Address::LookupAnyIPAddress("www.baidu.com:80");
//	//������ҵ�ַʧ�ܣ�����
//	if (!address)
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << "get address error";
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
//	}
//
//	shared_ptr<Socket> socket(new Socket(Socket::FamilyType(address->getFamily()), Socket::SocketType::TCP, 0));
//	bool return_value = socket->connect(address);
//	//���socket����ʧ�ܣ�����
//	if (!return_value)
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << "connect to " << address->toString() << " fail";
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//	}
//
//	shared_ptr<HttpConnection> connection(new HttpConnection(socket));
//
//	shared_ptr<HttpRequest> request(new HttpRequest);
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << "request: " << endl << request->toString();
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//	}
//
//	//request->setHeader("host", "www.baidu.com");
//	connection->sendRequest(request);
//
//
//	auto response = connection->receiveResponse();
//	//���û�н��յ���Ӧ������������
//	if (!response)
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << "receive response error";
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//		return;
//	}
//
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << "response: " << endl << response->toString();
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//	}
//
//
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << "========================";
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//	}
//
//	auto get_result = HttpConnection::DoGet("http://www.baidu.com", 300);
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream()
//			<< "result=" << get_result->m_result
//			<< " error=" << get_result->m_error
//			<< " response=" << (get_result->m_response ? get_result->m_response->toString() : "nullptr");
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//	}
//
//
//
//	{
//		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//		log_event->getSstream() << "========================";
//		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
//	}
//
//
//	test_pool();
// }
//
// int main(int argc, char** argv)
//{
//	IOManager iomanager(2);
//	iomanager.schedule(run);
//	return 0;
// }
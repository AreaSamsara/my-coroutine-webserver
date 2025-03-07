// #include "tcp-ip/address.h"
// #include "common/log.h"
// #include "common/singleton.h"
// #include "tcp-ip/socket.h"
// #include "concurrent/iomanager.h"

// using namespace AddressSpace;
// using namespace LogSpace;
// using namespace SingletonSpace;
// using namespace SocketSpace;
// using namespace IOManagerSpace;

// void test_socket()
// {
//     // 创建地址
//     shared_ptr<IPAddress> address = Address::LookupAnyIPAddress("www.baidu.com");
//     if (address)
//     {
//         shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//         log_event->getSstream() << "get address: " << address->toString();
//         Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
//     }
//     else
//     {
//         shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//         log_event->getSstream() << "get address fail " << address->toString();
//         Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
//         return;
//     }

//     // 设置端口号
//     address->setPort(80);

//     // 创建socket对象
//     shared_ptr<Socket> socket(new Socket((Socket::FamilyType)address->getFamily(), (Socket::SocketType)Socket::TCP));
//     {
//         shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//         log_event->getSstream() << "socket=" << socket->getSocket();
//         Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_DEBUG, log_event);
//     }

//     // socket对象尝试连接到目标地址
//     if (socket->connect(address))
//     {
//         shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//         log_event->getSstream() << "connect success,address=" << address->toString();
//         Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
//     }
//     else
//     {
//         shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//         log_event->getSstream() << "connect fail,address=" << address->toString();
//         Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
//         return;
//     }

//     // 发送请求报文
//     const char message[] = "GET / HTTP/1.0\r\n\r\n";
//     int return_value = socket->send(message, sizeof(message));
//     if (return_value <= 0)
//     {
//         shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//         log_event->getSstream() << "send fail,return_value=" << return_value;
//         Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
//         return;
//     }

//     // 接受响应
//     string buffers;
//     buffers.resize(4096);
//     return_value = socket->recv(&buffers[0], buffers.size());
//     if (return_value <= 0)
//     {
//         shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//         log_event->getSstream() << "receive fail,return_value=" << return_value;
//         Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
//         return;
//     }

//     buffers.resize(return_value);

//     shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
//     log_event->getSstream() << buffers;
//     Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_INFO, log_event);
// }

// int main(int argc, char **argv)
// {
//     IOManager iomanager;
//     iomanager.schedule(test_socket);
//     return 0;
// }
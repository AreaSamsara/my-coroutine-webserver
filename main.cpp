#include "http_parser.h"
#include "log.h"
#include "singleton.h"
#include "utility.h"

using namespace LogSpace;
using namespace SingletonSpace;
using namespace UtilitySpace;
using namespace HttpSpace;

const char test_request_data[] =
"POST / HTTP/1.1\r\n"
"Host: www.sylar.top\r\n"
"Content-Length: 10\r\n\r\n"
"1234567890";


void test_request()
{
	HttpRequestParser parser;
	string tempstr = test_request_data;
	size_t return_value = parser.execute(&tempstr[0], tempstr.size());

	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));

	log_event->getSstream() << "\nexecute return_value=" << return_value
		<< " has_error=" << parser.hasError()
		<< " is_finished=" << parser.isFinished()
		<< " total=" << tempstr.size()
		<< " content_length=" << parser.getContentLength() << endl;
	tempstr.resize(tempstr.size() - return_value);
	log_event->getSstream() << parser.getRequest()->toString() << endl
		<< tempstr;

	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
}

const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
"Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
"Server: Apache\r\n"
"Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
"ETag: \"51-47cf7e6ee8400\"\r\n"
"Accept-Ranges: bytes\r\n"
"Content-Length: 81\r\n"
"Cache-Control: max-age=86400\r\n"
"Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
"Connection: Close\r\n"
"Content-Type: text/html\r\n\r\n"
"<html>\r\n"
"<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
"</html>\r\n";

void test_response()
{
	HttpResponseParser parser;
	string tempstr = test_response_data;
	size_t return_value = parser.execute(&tempstr[0], tempstr.size());

	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));

	log_event->getSstream() << "\nexecute return_value=" << return_value
		<< " has_error=" << parser.hasError()
		<< " is_finished=" << parser.isFinished()
		<< " total=" << tempstr.size()
		<< " content_length=" << parser.getContentLength() << endl;
	tempstr.resize(tempstr.size() - return_value);
	log_event->getSstream() << parser.getResponse()->toString() << endl
		<< tempstr;

	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
}

int main(int argc, char** argv)
{
	test_request();

	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
	log_event->getSstream() << "--------------";
	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

	test_response();

	return 0;
}
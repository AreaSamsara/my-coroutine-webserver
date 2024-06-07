#include "http_session.h"
#include "http_parser.h"
#include "log.h"
#include "utility.h"
#include "singleton.h"

namespace HttpSessionSpace
{
	using namespace LogSpace;
	using namespace UtilitySpace;
	using namespace SingletonSpace;
	using std::stringstream;
	
	HttpSession::HttpSession(shared_ptr<Socket> socket, const bool is_socket_owner)
		:SocketStream(socket, is_socket_owner) {}

	shared_ptr<HttpRequest> HttpSession::receiveRequest()
	{
		shared_ptr<HttpRequestParser> parser(new HttpRequestParser);
		uint64_t buffer_size = HttpRequestParser::GetHttp_request_buffer_size();
		//uint64_t buffer_size = 100;

		shared_ptr<char> buffer(new char[buffer_size], [](char* ptr) {delete[]ptr; });

		char* data = buffer.get();
		int offset = 0;

		while (true)
		{
			int length = read(data + offset, buffer_size - offset);
			if (length <= 0)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "length <= 0";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);

				close();
				return nullptr;
			}
			length += offset;

			int parser_length = parser->execute(data, length);
			if (parser->hasError())
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "parser->hasError()";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);

				close();
				return nullptr;
			}

			offset = length - parser_length;

			if (offset == buffer_size)
			{
				shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
				log_event->getSstream() << "offset == buffer_size";
				Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::DEBUG, log_event);

				close();
				return nullptr;
			}
			if (parser->isFinished())
			{
				break;
			}
		}

		uint64_t content_length = parser->getContentLength();
		if (content_length > 0)
		{
			string body;
			body.resize(content_length);
			
			int length = 0;
			if (content_length >= offset)
			{
				memcpy(&body[0], data, offset);
				length = offset;
			}
			else
			{
				memcpy(&body[0], data, content_length);
				length = content_length;
			}

			content_length -= offset;
			if (content_length > 0)
			{
				if (read_fixed_size(&body[length], content_length) <= 0)
				{
					close();
					return nullptr;
				}
			}
			parser->getRequest()->setBody(body);
		}

		return parser->getRequest();
	}
	int HttpSession::sendResponse(shared_ptr<HttpResponse> response)
	{
		stringstream ss;
		ss << response->toString();
		string data = ss.str();
		return write_fixed_size(data.c_str(), data.size());
	}
}
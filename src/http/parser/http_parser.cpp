#include "http/parser/http_parser.h"
#include "common/log.h"
#include "common/utility.h"
#include "common/singleton.h"

namespace HttpSpace
{
    using namespace LogSpace;
    using namespace UtilitySpace;
    using namespace SingletonSpace;

    void on_request_method(void *data, const char *at, size_t length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        HttpMethod method = CharsToHttpMethod(at);

        // ����ǷǷ�method��ֱ�ӷ��ز���������
        if (method == HttpMethod::INVALID_METHOD)
        {
            shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
            log_event->getSstream() << "invalid http request method " << string(at, length);
            Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_WARN, log_event);
            parser->setError(1000);
            return;
        }
        parser->getRequest()->setMethod(method);
    }
    void on_request_uri(void *data, const char *at, size_t length)
    {
    }
    void on_request_fragment(void *data, const char *at, size_t length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        parser->getRequest()->setFragment(string(at, length));
    }
    void on_request_path(void *data, const char *at, size_t length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        parser->getRequest()->setPath(string(at, length));
    }
    void on_request_query(void *data, const char *at, size_t length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        parser->getRequest()->setQuery(string(at, length));
    }
    void on_request_version(void *data, const char *at, size_t length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        uint8_t version = 0;
        if (strncmp(at, "HTTP/1.1", length) == 0)
        {
            version = 0x11;
        }
        else if (strncmp(at, "HTTP/1.0", length) == 0)
        {
            version = 0x10;
        }
        // ����ǷǷ�HTTP�汾��ֱ�ӷ��ز���������
        else
        {
            shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
            log_event->getSstream() << "invalid http request version: " << string(at, length);
            Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_WARN, log_event);
            parser->setError(1001);
            return;
        }
        parser->getRequest()->setVersion(version);
    }
    void on_request_header_done(void *data, const char *at, size_t length)
    {
    }
    void on_request_http_field(void *data, const char *field, size_t field_length, const char *value, size_t value_length)
    {
        HttpRequestParser *parser = static_cast<HttpRequestParser *>(data);
        if (field_length == 0)
        {
            shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
            log_event->getSstream() << "invalid http request field length == 0";
            Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_WARN, log_event);
            // parser->setError(1002);   //��ʱ��Ը������Ҳ��Ҫ������Ϣ
            return;
        }
        parser->getRequest()->setHeader(string(field, field_length), string(value, value_length));
    }

    // class HttpRequestParser:public
    HttpRequestParser::HttpRequestParser() : m_error(0)
    {
        m_request.reset(new HttpRequest);
        http_parser_init(&m_parser);
        m_parser.request_method = on_request_method;
        m_parser.request_uri = on_request_uri;
        m_parser.fragment = on_request_fragment;
        m_parser.request_path = on_request_path;
        m_parser.query_string = on_request_query;
        m_parser.http_version = on_request_version;
        m_parser.header_done = on_request_header_done;
        m_parser.http_field = on_request_http_field;
        m_parser.data = this;
    }
    size_t HttpRequestParser::execute(char *data, size_t length)
    {
        size_t offset = http_parser_execute(&m_parser, data, length, 0);
        memmove(data, data + offset, (length - offset));
        return offset;
    }
    int HttpRequestParser::isFinished()
    {
        return http_parser_finish(&m_parser);
    }
    int HttpRequestParser::hasError()
    {
        return (m_error || http_parser_has_error(&m_parser));
    }

    uint64_t HttpRequestParser::getContentLength()
    {
        return m_request->getHeaderAs<uint64_t>("content-length", 0);
    }

    // class HttpRequestParser:private static variable
    uint64_t HttpRequestParser::s_http_request_buffer_size = 4 * 1024ull;
    uint64_t HttpRequestParser::s_http_request_max_body_size = 64 * 1024 * 1024ull;

    void on_response_reason(void *data, const char *at, size_t length)
    {
        HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
        parser->getResponse()->setReason(string(at, length));
    }
    void on_response_status(void *data, const char *at, size_t length)
    {
        HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
        HttpStatus status = (HttpStatus)(atoi(at));
        parser->getResponse()->setStatus(status);
    }
    void on_response_chunk(void *data, const char *at, size_t length)
    {
    }
    void on_response_version(void *data, const char *at, size_t length)
    {
        HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
        uint8_t version = 0;
        if (strncmp(at, "HTTP/1.1", length) == 0)
        {
            version = 0x11;
        }
        else if (strncmp(at, "HTTP/1.0", length) == 0)
        {
            version = 0x10;
        }
        // ����ǷǷ�HTTP�汾��ֱ�ӷ��ز���������
        else
        {
            shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
            log_event->getSstream() << "invalid http response version " << string(at, length);
            Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_WARN, log_event);
            parser->setError(1001);
            return;
        }
        parser->getResponse()->setVersion(version);
    }
    void on_response_header_done(void *data, const char *at, size_t length)
    {
    }
    void on_response_last_chunk(void *data, const char *at, size_t length)
    {
    }
    void on_response_http_field(void *data, const char *field, size_t field_length, const char *value, size_t value_length)
    {
        HttpResponseParser *parser = static_cast<HttpResponseParser *>(data);
        if (field_length == 0)
        {
            shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
            log_event->getSstream() << "invalid http response field length == 0";
            Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_WARN, log_event);
            // parser->setError(1002);   //��ʱ��Ը������Ҳ��Ҫ������Ϣ
            return;
        }
        parser->getResponse()->setHeader(string(field, field_length), string(value, value_length));
    }

    // class HttpResponseParser:public
    HttpResponseParser::HttpResponseParser() : m_error(0)
    {
        m_response.reset(new HttpResponse);
        httpclient_parser_init(&m_parser);
        m_parser.reason_phrase = on_response_reason;
        m_parser.status_code = on_response_status;
        m_parser.chunk_size = on_response_chunk;
        m_parser.http_version = on_response_version;
        m_parser.header_done = on_response_header_done;
        m_parser.last_chunk = on_response_last_chunk;
        m_parser.http_field = on_response_http_field;
        m_parser.data = this;
    }
    size_t HttpResponseParser::execute(char *data, size_t length, const bool chunk)
    {
        if (chunk)
        {
            httpclient_parser_init(&m_parser);
        }
        size_t offset = httpclient_parser_execute(&m_parser, data, length, 0);
        memmove(data, data + offset, (length - offset));
        return offset;
    }
    int HttpResponseParser::isFinished()
    {
        return httpclient_parser_finish(&m_parser);
    }
    int HttpResponseParser::hasError()
    {
        return (m_error || httpclient_parser_has_error(&m_parser));
    }

    uint64_t HttpResponseParser::getContentLength()
    {
        return m_response->getHeaderAs<uint64_t>("content-length", 0);
    }

    // class HttpResponseParser:private static variable
    uint64_t HttpResponseParser::s_http_response_buffer_size = 4 * 1024ull;
    uint64_t HttpResponseParser::s_http_response_max_body_size = 64 * 1024 * 1024ull;
}
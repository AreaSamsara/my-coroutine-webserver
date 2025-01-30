#pragma once
#include "http/http.h"
#include "http/parser/http11_parser.h"
#include "http/parser/httpclient_parser.h"

namespace HttpSpace
{
    class HttpRequestParser
    {
    public:
        HttpRequestParser();
        size_t execute(char *data, size_t length);
        int isFinished();
        int hasError();

        shared_ptr<HttpRequest> getRequest() { return m_request; }
        const http_parser &getParser() const { return m_parser; }
        uint64_t getContentLength();

        void setError(const int error) { m_error = error; }

    public:
        static uint64_t GetHttp_request_buffer_size() { return s_http_request_buffer_size; }
        static uint64_t GetHttp_request_max_body_size() { return s_http_request_max_body_size; }

    private:
        http_parser m_parser;
        shared_ptr<HttpRequest> m_request;
        /// ������
        /// 1000: invalid method
        /// 1001: invalid version
        /// 1002: invalid field
        int m_error;

    private:
        static uint64_t s_http_request_buffer_size;
        static uint64_t s_http_request_max_body_size;
    };

    class HttpResponseParser
    {
    public:
        HttpResponseParser();
        // chunk��ʾ��Ӧ�Ƿ�ֶ�
        size_t execute(char *data, size_t length, const bool chunk);
        int isFinished();
        int hasError();

        shared_ptr<HttpResponse> getResponse() { return m_response; }
        const httpclient_parser &getParser() const { return m_parser; }
        uint64_t getContentLength();

        void setError(const int error) { m_error = error; }

    public:
        static uint64_t GetHttp_response_buffer_size() { return s_http_response_buffer_size; }
        static uint64_t GetHttp_response_max_body_size() { return s_http_response_max_body_size; }

    private:
        httpclient_parser m_parser;
        shared_ptr<HttpResponse> m_response;
        /// ������
        /// 1001: invalid version
        /// 1002: invalid field
        int m_error;

    private:
        static uint64_t s_http_response_buffer_size;
        static uint64_t s_http_response_max_body_size;
    };
}
#pragma once
#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace HttpSpace
{
    class HttpRequestParser
    {
    public:
        HttpRequestParser();
        size_t execute(char* data, size_t length);
        int isFinished();
        int hasError();

        shared_ptr<HttpRequest> getRequest() { return m_request; }
        void setError(const int error) { m_error = error; }

        //未达到预期效果，原因不明
        uint64_t getContentLength();
    private:
        http_parser m_parser;
        shared_ptr<HttpRequest> m_request;
        /// 错误码
        /// 1000: invalid method
        /// 1001: invalid version
        /// 1002: invalid field
        int m_error;
    };

    class HttpResponseParser
    {
    public:
        HttpResponseParser();
        size_t execute(char* data, size_t length);
        int isFinished();
        int hasError();

        shared_ptr<HttpResponse> getResponse() { return m_response; }
        void setError(const int error) { m_error = error; }

        //未达到预期效果，原因不明
        uint64_t getContentLength();
    private:
        httpclient_parser  m_parser;
        shared_ptr<HttpResponse> m_response;
        /// 错误码
        /// 1001: invalid version
        /// 1002: invalid field
        int m_error;
    };
}
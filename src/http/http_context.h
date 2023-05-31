#pragma once

#include "http_request.h"

class Buffer;

class Http_Context
{
public:
    enum Http_Request_Parse_State
    {
        EXPECT_REQUESR_LINE,
        EXPECT_HEADERS,
        EXPECT_BODY,
        GOT_ALL,
    };

    Http_Context() : state_(EXPECT_REQUESR_LINE) {}

    bool got_all() const { return state_ == GOT_ALL; }

    void reset()
    {
        state_ = EXPECT_REQUESR_LINE;
        Http_Request dummy;
        request_.swap(dummy);
    }

    const Http_Request& request() const { return request_; }

    Http_Request& request() { return request_; }

    bool parse_request(Buffer* buf, Timestamp receive_time);

private:
    bool process_request_line(const char* begin, const char* end);

    Http_Request_Parse_State state_;
    Http_Request request_;
};
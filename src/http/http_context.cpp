#include "http_context.h"
#include "net/buffer.h"

#include <algorithm>
#include <string>

bool Http_Context::process_request_line(const char* begin, const char* end)
{
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    if (space != end && request_.set_method(start, space)) {
        start = space+1;
        space = std::find(start, end, ' ');
        if (space != end) {
            const char* question = std::find(start, space, '?');
            if (question != space) {
                request_.set_path(start, question);
                request_.set_query(question, space);
            }
            else {
                request_.set_path(start, space);
            }
            start = space+1;
            succeed = end-start == 8 && std::equal(start, end-1, "HTTP/1.");
            if (succeed) {
                if (*(end-1) == '1')
                    request_.set_version(Version::HTTP11);
                else if (*(end-1) == '0')
                    request_.set_version(Version::HTTP10);
                else
                    succeed = false;
            }
        }
    }
    return succeed;
}

bool Http_Context::parse_request(Buffer* buf, Timestamp receive_time)
{
    bool ok = true;
    bool has_more = true;
    while (has_more) {
        if (state_ == EXPECT_REQUESR_LINE) {
            const char* crlf = buf->find_CRLF();
            if (crlf) {
                ok = process_request_line(buf->peek(), crlf);
                if (ok) {
                    request_.set_receive_time(receive_time);
                    buf->retrieve_until(crlf + 2);
                    state_ = EXPECT_HEADERS;
                } else
                    has_more = false;
            } else
                has_more = false;
        } else if (state_ == EXPECT_HEADERS) {
            const char* crlf = buf->find_CRLF();
            if (crlf) {
                const char* colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf)
                    request_.add_header(buf->peek(), colon, crlf);
                else {
                    // empty line, end of header
                    state_ = GOT_ALL;
                    has_more = false;
                }
                buf->retrieve_until(crlf + 2);
            } else
                has_more = false;
        } else if (state_ == EXPECT_BODY) {
        // TO DO
        }
    }
    return ok;
}
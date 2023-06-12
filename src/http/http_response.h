#pragma once

#include "http_common_define.h"

#include <map>
#include <string>

using std::string;

class Buffer;
class Http_Response
{
public:
    enum Http_Status_Code
    {
        UNKNOWN,
        OK = 200,
        MOVE_PERMANENTLY = 301,
        BAD_REQUEST = 400,
        NOT_FOUND = 404,
    };

    explicit Http_Response(bool close) :
        status_code_(UNKNOWN),
        close_connection_(close),
        version_(Version::UNKNOWN) {}

    void set_status_code(Http_Status_Code code) { status_code_ = code; }

    void set_status_message(const string& message) { status_message_ = message; }

    void set_close_connection(bool on) { close_connection_ = on; }

    bool close_connection() const{ return close_connection_; }

    void set_content_type(const string& content_type) { add_header("Content-Type", content_type); }

    void set_version(Version v) { version_ = v; }

    Version get_version() const { return version_; }

    void add_header(const string& key, const string& value) { headers_[key] = value; }

    void set_body(const string& body) { body_ = body; }

    void append_to_buffer(Buffer* output) const;

private:
    std::map<string, string> headers_;
    Http_Status_Code status_code_;
    string status_message_;
    bool close_connection_;
    Version version_;
    string body_;
};
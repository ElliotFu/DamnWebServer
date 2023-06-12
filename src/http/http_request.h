#pragma once

#include "http_common_define.h"
#include "base/timestamp.h"

#include <cassert>
#include <map>
#include <string>

using std::string;

class Http_Request
{
public:
    enum Method
    {
        INVALID, GET, POST, HEAD, PUT, DELETE
    };

    Http_Request() : method_(INVALID), version_(UNKNOWN) {}

    void set_version(Version v) { version_ = v; }

    Version get_version() const { return version_; }

    bool set_method(const char* start, const char* end)
    {
        assert(method_ == INVALID);
        string m(start, end);
        if (m == "GET")
            method_ = GET;
        else if (m == "POST")
            method_ = POST;
        else if (m == "HEAD")
            method_ = HEAD;
        else if (m == "PUT")
            method_ = PUT;
        else if (m == "DELETE")
            method_ = DELETE;
        else
            method_ = INVALID;

        return method_ != INVALID;
    }

    Method method() const { return method_; }

    const char* method_string() const
    {
        const char* result = "UNKNOWN";
        switch(method_)
        {
        case GET:
            result = "GET";
            break;
        case POST:
            result = "POST";
            break;
        case HEAD:
            result = "HEAD";
            break;
        case PUT:
            result = "PUT";
            break;
        case DELETE:
            result = "DELETE";
            break;
        default:
            break;
        }

        return result;
    }

    void set_path(const char* start, const char* end) { path_.assign(start, end); }

    const string& path() const { return path_; }

    void set_query(const char* start, const char* end) { query_.assign(start, end); }

    const string& query() const { return query_; }

    void set_receive_time(Timestamp t) { receive_time_ = t; }

    Timestamp receive_time() const { return receive_time_; }

    void add_header(const char* start, const char* colon, const char* end)
    {
        string field(start, colon);
        ++colon;
        while (colon < end && isspace(*colon))
            ++colon;
        string value(colon, end);
        while (!value.empty() && isspace(value[value.size()-1]))
            value.resize(value.size()-1);

        headers_[field] = value;
    }

    string get_header(const string& field) const
    {
        string result;
        std::map<string, string>::const_iterator it = headers_.find(field);
        if (it != headers_.end())
            result = it->second;

        return result;
    }

    const std::map<string, string>& headers() const { return headers_; }

    void swap(Http_Request& that)
    {
        std::swap(method_, that.method_);
        std::swap(version_, that.version_);
        path_.swap(that.path_);
        query_.swap(that.query_);
        receive_time_.swap(that.receive_time_);
        headers_.swap(that.headers_);
    }

private:
    Method method_;
    Version version_;
    string path_;
    string query_;
    Timestamp receive_time_;
    std::map<string, string> headers_;
};
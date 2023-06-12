#pragma once

#include "base/noncopyable.h"
#include "net/tcp_server.h"

#include <arpa/inet.h>
#include <functional>
#include <string>

class Event_Loop;
class Http_Request;
class Http_Response;

class Http_Server : Noncopyable
{
public:
    using Http_Callback = std::function<void(const Http_Request&, Http_Response*)>;
    Http_Server(Event_Loop* loop,
                const sockaddr_in& listen_addr,
                const std::string& name);
    ~Http_Server() = default;

    Event_Loop* get_loop() const { return server_.get_loop(); }

    void set_http_callback(const Http_Callback& cb) { http_callback_ = cb; }
    void set_thread_num(int num_threads) { server_.set_thread_num(num_threads); }
    void start();

private:
    void on_connection(const Tcp_Server::Tcp_Connection_Ptr& conn);
    void on_message(const Tcp_Server::Tcp_Connection_Ptr& conn,
                    Buffer* buf,
                    Timestamp receive_time);
    void on_request(const Tcp_Server::Tcp_Connection_Ptr&, const Http_Request&);

    Tcp_Server server_;
    Http_Callback http_callback_;
};
#pragma once

#include "base/noncopyable.h"
#include "channel.h"
#include <functional>
#include <netinet/ip.h>

class Event_Loop;

class Acceptor : Noncopyable
{
public:
    typedef std::function<void(int sockfd, const sockaddr_in&)> New_Connection_Callback;

    Acceptor(Event_Loop* loop, const sockaddr_in& listen_addr);
    ~Acceptor();

    void set_new_connection_callback(const New_Connection_Callback& cb) { new_connection_callback_ = cb; }
    bool listenning() const { return listenning_; }
    void listen();

private:
    void handle_read();
    int accept(sockaddr_in& addr);

    Event_Loop* loop_;
    int listenfd_;
    Channel listen_channel_;
    New_Connection_Callback new_connection_callback_;
    bool listenning_;
    int idle_fd_;
};
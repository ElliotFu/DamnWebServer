#pragma once

#include "base/noncopyable.h"
#include "base/timestamp.h"
#include "event_loop_thread_pool.h"
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <netinet/ip.h>

class Acceptor;
class Buffer;
class Event_Loop;
class Tcp_Connection;

class Tcp_Server : Noncopyable
{
public:
    typedef std::shared_ptr<Tcp_Connection> Tcp_Connection_Ptr;
    typedef std::function<void (const Tcp_Connection_Ptr&)> Connection_Callback;
    // the data has been read to (buf, len)
    typedef std::function<void (const Tcp_Connection_Ptr&,
                                Buffer*,
                                Timestamp)> Message_Callback;

    Tcp_Server(Event_Loop* loop, const sockaddr_in& listen_addr, const std::string& name);
    ~Tcp_Server();

    Event_Loop* get_loop() const { return loop_; }

    void set_thread_num(int num_threads) { thread_pool_->set_thread_num(num_threads); }
    void start();
    void set_connection_callback(const Connection_Callback& cb) { connection_callback_ = cb; }
    void set_message_callback(const Message_Callback& cb) { message_callback_ = cb; }

private:
    void new_connection(int sockfd, const sockaddr_in& peer_addr);
    void remove_connection(const Tcp_Connection_Ptr& conn);
    void remove_connection_in_loop(const Tcp_Connection_Ptr& conn);

    typedef std::map<std::string, Tcp_Connection_Ptr> Connection_Map;
    Event_Loop* loop_;
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<Event_Loop_Thread_Pool> thread_pool_;
    Connection_Callback connection_callback_;
    Message_Callback message_callback_;
    std::atomic_bool started_;
    int next_conn_id_;
    Connection_Map connections_;
};
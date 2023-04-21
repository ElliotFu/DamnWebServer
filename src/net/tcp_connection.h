#pragma once

#include "base/noncopyable.h"
#include "base/timestamp.h"
#include "buffer.h"
#include "tcp_server.h"
#include <atomic>
#include <arpa/inet.h>
#include <memory>
#include <string>

class Channel;
class Event_Loop;

class Tcp_Connection : Noncopyable,
                       public std::enable_shared_from_this<Tcp_Connection>
{
public:
    /// Constructs a TcpConnection with a connected sockfd
    ///
    /// User should not create this object.
    Tcp_Connection(Event_Loop* loop,
                   const std::string& name,
                   int sockfd,
                   const sockaddr_in& local_addr,
                   const sockaddr_in& peer_addr);
    ~Tcp_Connection();

    Event_Loop* get_loop() const { return loop_; }
    const std::string& name() const { return name_; }
    const sockaddr_in& local_address() const { return local_addr_; }
    const sockaddr_in& peer_address() const { return peer_addr_; }
    bool connected() const { return state_ == k_connected; }
    bool disconnected() const { return state_ == k_disconnected; }

    typedef std::function<void (const Tcp_Server::Tcp_Connection_Ptr&)> Close_Callback;
    typedef std::function<void (const Tcp_Server::Tcp_Connection_Ptr&)> Connection_Callback;

    void set_connection_callback(const Connection_Callback& cb) { connection_callback_ = cb; }
    void set_message_callback(const Tcp_Server::Message_Callback& cb) { message_callback_ = cb; }
    void set_close_callback(const Close_Callback& cb) { close_callback_ = cb; }

    void connect_established();
    void connect_destroyed();

    void send(const std::string& message);
    void shutdown();

    void set_tcp_no_delay(bool on);

private:
    enum State_E { k_disconnected, k_connecting, k_connected, k_disconnecting };

    void set_state(State_E s) { state_ = s; }
    void handle_read(Timestamp receive_time);
    void handle_write();
    void handle_close();
    void handle_error();
    void send_in_loop(const std::string& message);
    void shut_down_in_loop();

    Event_Loop* loop_;
    std::string name_;
    std::atomic_int state_;
    bool reading_;

    int socket_fd_;
    std::shared_ptr<Channel> channel_;
    sockaddr_in local_addr_;
    sockaddr_in peer_addr_;
    Tcp_Server::Connection_Callback connection_callback_;
    Tcp_Server::Message_Callback message_callback_;
    Close_Callback close_callback_;

    Buffer input_buffer_;
    Buffer output_buffer_;
};
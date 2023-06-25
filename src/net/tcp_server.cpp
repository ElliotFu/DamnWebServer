#include "tcp_server.h"
#include "acceptor.h"
#include "channel.h"
#include "tcp_connection.h"
#include "event_loop.h"
#include "event_loop_thread_pool.h"
#include "base/log/logger.h"
#include <arpa/inet.h>
#include <functional>
#include <string.h>

void default_connection_callback(const Tcp_Server::Tcp_Connection_Ptr& conn)
{
    char local_ip[INET_ADDRSTRLEN], peer_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &conn->local_address().sin_addr, local_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &conn->peer_address().sin_addr, peer_ip, INET_ADDRSTRLEN);
    printf("%s:%d -> %s:%d is %s\n",
            local_ip, ntohs(conn->local_address().sin_port),
            peer_ip, ntohs(conn->peer_address().sin_port),
            conn->connected() ? "UP" : "DOWN");
    // LOG_TRACE << conn->localAddress().toIpPort() << " -> "
    //           << conn->peerAddress().toIpPort() << " is "
    //           << (conn->connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void default_message_callback(const Tcp_Server::Tcp_Connection_Ptr&, Buffer* buf, Timestamp)
{
    buf->retrieve_all();
}

Tcp_Server::Tcp_Server(Event_Loop* loop,
                       const sockaddr_in& listenAddr,
                       const std::string& name)
    : loop_(loop),
      name_(name),
      acceptor_(new Acceptor(loop, listenAddr)),
      thread_pool_(new Event_Loop_Thread_Pool(loop)),
      connection_callback_(default_connection_callback),
      message_callback_(default_message_callback),
      started_(false),
      next_conn_id_(1)
{
    acceptor_->set_new_connection_callback(std::bind(&Tcp_Server::new_connection,
                                                     this,
                                                     std::placeholders::_1,
                                                     std::placeholders::_2));
}

Tcp_Server::~Tcp_Server()
{
    loop_->assert_in_loop_thread();
    printf("TcpServer::~TcpServer [%s] destructing\n", name_.c_str());
    // LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for (auto& item : connections_)
    {
        Tcp_Connection_Ptr conn(item.second);
        item.second.reset();
        conn->get_loop()->run_in_loop(
            std::bind(&Tcp_Connection::connect_destroyed, conn));
    }
}

void Tcp_Server::start()
{
    if (started_.exchange(true) == false)
    {
        thread_pool_->start();
        assert(!acceptor_->listenning());
        loop_->run_in_loop(
            std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

void Tcp_Server::new_connection(int sockfd, const sockaddr_in& peer_addr)
{
    loop_->assert_in_loop_thread();
    char buf[32];
    snprintf(buf, sizeof buf, "#%d", next_conn_id_);
    ++next_conn_id_;
    std::string conn_name = name_ + buf;

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer_addr.sin_addr, ip, INET_ADDRSTRLEN);
    printf("Tcp_Server::new_connection [%s] - new connection [%s] from %s:%d\n",
            name_.c_str(), conn_name.c_str(), ip, ntohs(peer_addr.sin_port));
    LOG_INFO << "TcpServer::newConnection [" << name_
             << "] - new connection [" << conn_name
             << "] from " << ip << ':' << ntohs(peer_addr.sin_port);

    sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof local_addr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof local_addr);
    if (::getsockname(sockfd,
                      static_cast<sockaddr*>((void*)(&local_addr)),
                      &addrlen) < 0)
    {
        printf("ERROR: Tcp_Connection::new_connection - sockets::getLocalAddr\n");
        // LOG_SYSERR << "sockets::getLocalAddr";
    }
    Event_Loop* io_loop = thread_pool_->get_next_loop();
    Tcp_Connection_Ptr conn(
        new Tcp_Connection(io_loop, conn_name, sockfd, local_addr, peer_addr));
    connections_[conn_name] = conn;
    conn->set_connection_callback(connection_callback_);
    conn->set_message_callback(message_callback_);
    conn->set_close_callback(std::bind(&Tcp_Server::remove_connection, this, std::placeholders::_1));
    io_loop->run_in_loop(std::bind(&Tcp_Connection::connect_established, conn));
}

void Tcp_Server::remove_connection(const Tcp_Connection_Ptr& conn)
{
    loop_->run_in_loop(std::bind(&Tcp_Server::remove_connection_in_loop, this, conn));
}

void Tcp_Server::remove_connection_in_loop(const Tcp_Connection_Ptr& conn)
{
    loop_->assert_in_loop_thread();
    printf("Tcp_Server::remove_connection_in_loop [%s] - connection %s\n", name_.c_str(), conn->name().c_str());
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();
    size_t n = connections_.erase(conn->name());
    assert(n == 1);
    Event_Loop* io_loop = conn->get_loop();
    io_loop->queue_in_loop(std::bind(&Tcp_Connection::connect_destroyed, conn));
}
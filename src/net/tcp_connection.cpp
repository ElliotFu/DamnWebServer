#include "tcp_connection.h"
#include "channel.h"
#include "event_loop.h"
#include <netinet/tcp.h>
#include <string.h>
#include <unistd.h>

Tcp_Connection::Tcp_Connection(Event_Loop* loop,
                               const std::string& name_arg,
                               int sockfd,
                               const sockaddr_in& local_addr,
                               const sockaddr_in& peer_addr)
  : loop_(loop),
    name_(name_arg),
    state_(k_connecting),
    reading_(true),
    socket_fd_(sockfd),
    channel_(new Channel(loop, sockfd)),
    local_addr_(local_addr),
    peer_addr_(peer_addr)
{
    channel_->set_read_callback(std::bind(&Tcp_Connection::handle_read, this, std::placeholders::_1));
    channel_->set_write_callback(std::bind(&Tcp_Connection::handle_write, this));
    channel_->set_close_callback(std::bind(&Tcp_Connection::handle_close, this));
    channel_->set_error_callback(std::bind(&Tcp_Connection::handle_error, this));
    // LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
    //           << " fd=" << sockfd;

    // set keepalive
    int optval = 1;
    ::setsockopt(socket_fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}

Tcp_Connection::~Tcp_Connection()
{
    // LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
    //           << " fd=" << channel_->fd()
    //           << " state=" << stateToString();
    assert(state_ == k_disconnected);
}

void Tcp_Connection::handle_read(Timestamp receive_time)
{
    loop_->assert_in_loop_thread();
    int saved_errno = 0;
    ssize_t n = input_buffer_.read_fd(channel_->fd(), &saved_errno);
    if (n > 0) {
        message_callback_(shared_from_this(), &input_buffer_, receive_time);
    }
    else if (n == 0)
        handle_close();
    else {
        errno = saved_errno;
        printf("Tcp_Connection::handle_read\n");
        handle_error();
        // LOG_SYSERR << "Tcp_Connection::handle_read";
    }
}

void Tcp_Connection::handle_write()
{
    loop_->assert_in_loop_thread();
    if (channel_->is_writing()) {
        ssize_t n = ::write(channel_->fd(), output_buffer_.peek(), output_buffer_.readable_bytes());
        if (n > 0) {
            output_buffer_.retrieve(n);
            if (output_buffer_.readable_bytes() == 0) {
                channel_->disable_writing();
                if (state_ == k_disconnecting)
                    shut_down_in_loop();
            }
        } else {
            printf("ERROR: Tcp_Connection::handleWrite\n");
            // LOG_SYSERR << "TcpConnection::handleWrite";
        }
    } else {
        printf("Connection fd = %d is down, no more writing\n", channel_->fd());
        // LOG_TRACE << "Connection fd = " << channel_->fd() << " is down, no more writing";
    }
}

void Tcp_Connection::handle_close()
{
    loop_->assert_in_loop_thread();
    printf("Tcp_Connection::handle_close fd = %d, state = %d\n", channel_->fd(), state_.load());
    // LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
    assert(state_ == k_connected || state_ == k_disconnecting);
    set_state(k_disconnected);
    channel_->disable_all();
    connection_callback_(shared_from_this());
    close_callback_(shared_from_this());
}

void Tcp_Connection::handle_error()
{
    int err;
    socklen_t optlen = static_cast<socklen_t>(sizeof err);

    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &err, &optlen) < 0)
        err = errno;

    char errno_buf[512];
    printf("Tcp_Connection::handle_error [%s] - SO_ERROR = %d %s\n",
            name_.c_str(), err, strerror_r(err, errno_buf, sizeof errno_buf));
    // LOG_ERROR << "TcpConnection::handleError [" << name_
    //           << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

void Tcp_Connection::connect_destroyed()
{
    loop_->assert_in_loop_thread();
    if (state_.exchange(k_disconnected) == k_connected) {
        channel_->disable_all();
        connection_callback_(shared_from_this());
    }
    channel_->remove();
}

void Tcp_Connection::connect_established()
{
    loop_->assert_in_loop_thread();
    assert(state_ == k_connecting);
    set_state(k_connected);
    // channel_->tie(shared_from_this());
    channel_->enable_reading();
    connection_callback_(shared_from_this());
}

void Tcp_Connection::shutdown()
{
    if (state_.exchange(k_disconnecting) == k_connected) {
        loop_->run_in_loop(std::bind(&Tcp_Connection::shut_down_in_loop, shared_from_this()));
    }
}

void Tcp_Connection::shut_down_in_loop()
{
    loop_->assert_in_loop_thread();
    if (!channel_->is_writing()) {
        if (::shutdown(socket_fd_, SHUT_WR) < 0) {
            printf("ERROR: Tcp_Connection::shut_down_in_loop - shutdown\n");
            // LOG_SYSERR << "sockets::shutdownWrite";
        }
    }
}

void Tcp_Connection::send(const std::string& message)
{
    if (state_ == k_connected) {
        if (loop_->is_in_loop_thread())
            send_in_loop(message);
        else {
            void (Tcp_Connection::*fp)(const string&) = &Tcp_Connection::send_in_loop;
            loop_->run_in_loop(std::bind(fp, shared_from_this(), message));
        }
    }
}

void Tcp_Connection::send(Buffer* buf)
{
    if (state_ == k_connected)
    {
        if (loop_->is_in_loop_thread()) {
            send_in_loop(buf->peek(), buf->readable_bytes());
            buf->retrieve_all();
        } else {
            void (Tcp_Connection::*fp)(const char*, size_t) = &Tcp_Connection::send_in_loop;
            loop_->run_in_loop(std::bind(fp, shared_from_this(), buf->peek(), buf->readable_bytes()));
        }
    }
}

void Tcp_Connection::send_in_loop(const std::string& message)
{
    send_in_loop(message.data(), message.size());
}

void Tcp_Connection::send_in_loop(const char* data, size_t len)
{
    loop_->assert_in_loop_thread();
    ssize_t nwrote = 0;
    bool pipe_or_reset = false;
    if (state_.load() == k_disconnected) {
        // LOG_ERROR << "disconnected, give up writing.";
        return;
    }

    if (!channel_->is_writing() && output_buffer_.readable_bytes() == 0) {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            if (static_cast<size_t>(nwrote) < len) {
                printf("I am going to write more data\n");
                // LOG_TRACE << "I am going to write more data";
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                printf("ERROR: Tcp_Conn - send_in_loop\n");
                // LOG_SYSERR << "Tcp_Connection::send_in_loop";
                if (errno == EPIPE || errno == ECONNRESET) {
                    pipe_or_reset = true;
                }
            }
        }
    }

    assert(nwrote >= 0);
    if (!pipe_or_reset && static_cast<size_t>(nwrote) < len) {
        output_buffer_.append(data+nwrote, len-nwrote);
        if (!channel_->is_writing())
            channel_->enable_writing();
    }
}

void Tcp_Connection::set_tcp_no_delay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(socket_fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}
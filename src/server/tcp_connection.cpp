#include "tcp_connection.h"
#include "channel.h"
#include "event_loop.h"
#include <string.h>

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

}

void Tcp_Connection::handle_close()
{
    loop_->assert_in_loop_thread();
    printf("Tcp_Connection::handle_close state = %d\n", state_.load());
    // LOG_TRACE << "Tcp_Connection::handle_close state = " << state_;
    assert(state_ == k_connected);
    channel_->disable_all();
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
    assert(state_ == k_connected);
    set_state(k_disconnected);
    channel_->disable_all();
    connection_callback_(shared_from_this());
    loop_->remove_channel(channel_.get());
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
#include "acceptor.h"
#include "event_loop.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

Acceptor::Acceptor(Event_Loop* loop, const sockaddr_in& listen_addr)
    : loop_(loop),
      listenfd_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)),
      listen_channel_(loop, listenfd_),
      listenning_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    if (listenfd_ < 0) {
        printf("Error - Acceptor::Acceptor(): socket");
        // LOG_SYSFATAL << "Acceptor::socket";
    }

    int optval = 1;
    int ret = ::setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0) {
        printf("Error - Acceptor::Acceptor(): SO_REUSE_ADDR");
        // LOG_SYSERR << "SO_REUSEADDR failed.";
    }

    ret = ::setsockopt(listenfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0) {
        printf("Error - Acceptor::Acceptor(): SO_REUSE_PORT");
        // LOG_SYSERR << "SO_REUSEPORT failed.";
    }

    ret = ::bind(listenfd_,
                 static_cast<const sockaddr*>((void*)&listen_addr),
                 sizeof(listen_addr));
    if (ret < 0) {
        printf("Error - Acceptor::Acceptor(): bind");
        // LOG_SYSFATAL << "Acceptor::bind";
    }

    listen_channel_.set_read_callback(std::bind(&Acceptor::handle_read, this));
}

Acceptor::~Acceptor()
{
    listen_channel_.disable_all();
    listen_channel_.remove();
    ::close(listenfd_);
}

void Acceptor::listen()
{
    loop_->assert_in_loop_thread();
    listenning_ = true;
    int ret = ::listen(listenfd_, SOMAXCONN);
    if (ret < 0) {
        printf("Error - Acceptor::listen");
        // LOG_SYSFATAL << "Acceptor::listen";
    }
    listen_channel_.enable_reading();
}

void Acceptor::handle_read()
{
    loop_->assert_in_loop_thread();
    sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    int connfd = accept(peer_addr);
    if (connfd >= 0) {
        if (new_connection_callback_)
            new_connection_callback_(connfd, peer_addr);
        else
            ::close(connfd);
    } else {
        // LOG_SYSERR << "in Acceptor::handle_read";
        if (errno == EMFILE) {
            ::close(idle_fd_);
            idle_fd_ = ::accept(listenfd_, nullptr, nullptr);
            ::close(idle_fd_);
            idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}

int Acceptor::accept(sockaddr_in& addr)
{
    socklen_t addr_len = static_cast<socklen_t>(sizeof(addr));
#if VALGRIND || defined (NO_ACCEPT4)
    int connfd = ::accept(listenfd_, static_cast<sockaddr*>((void*)&addr), &addrlen);
    int flags = ::fcntl(connfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(connfd, F_SETFL, flags);

    // close-on-exec
    flags = ::fcntl(connfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(connfd, F_SETFD, flags);
#else
    int connfd = ::accept4(listenfd_, static_cast<sockaddr*>((void*)&addr), &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
    if (connfd < 0) {
        int saved_errno = errno;
        // LOG_SYSERR << "Socket::accept";
        switch (saved_errno) {
        case EAGAIN:
        case ECONNABORTED:
        case EINTR:
        case EPROTO: // ???
        case EPERM:
        case EMFILE: // per-process lmit of open file desctiptor ???
        // expected errors
            errno = saved_errno;
            break;
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
        // unexpected errors
            // LOG_FATAL << "unexpected error of ::accept " << saved_errno;
            break;
        default:
            // LOG_FATAL << "unknown error of ::accept " << saved_errno;
            break;
        }
    }

    return connfd;
}
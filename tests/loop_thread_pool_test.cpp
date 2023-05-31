#include "net/tcp_server.h"
#include "net/tcp_connection.h"
#include "net/event_loop.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void onConnection(const Tcp_Server::Tcp_Connection_Ptr& conn)
{
  if (conn->connected())
  {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &conn->peer_address().sin_addr, ip, INET_ADDRSTRLEN);
    printf("onConnection(): tid=%d new connection [%s] from %s:%d\n",
           Current_Thread::tid(),
           conn->name().c_str(),
           ip, ntohs(conn->peer_address().sin_port));
  }
  else
  {
    printf("onConnection(): tid=%d connection [%s] is down\n",
           Current_Thread::tid(),
           conn->name().c_str());
  }
}

void onMessage(const Tcp_Server::Tcp_Connection_Ptr& conn,
               Buffer* buf,
               Timestamp receiveTime)
{
  printf("onMessage(): tid=%d received %zd bytes from connection [%s] at %s\n",
         Current_Thread::tid(),
         buf->readable_bytes(),
         conn->name().c_str(),
         receiveTime.to_formatted_string().c_str());

  conn->send(buf->retrieve_all_as_string());
}

int main(int argc, char* argv[])
{
  printf("main(): pid = %d\n", getpid());

  sockaddr_in listen_addr;
  memset(&listen_addr, 0, sizeof(listen_addr));
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_addr.s_addr = INADDR_ANY;
  listen_addr.sin_port = htons(9981);
  Event_Loop loop;
  Tcp_Server server(&loop, listen_addr, "server_name");
  server.set_connection_callback(onConnection);
  server.set_message_callback(onMessage);
  if (argc > 1) {
    server.set_thread_num(atoi(argv[1]));
    printf("threads: %d\n", atoi(argv[1]));
  }
  server.start();

  loop.loop();
}
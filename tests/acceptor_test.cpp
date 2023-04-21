#include "net/acceptor.h"
#include "net/event_loop.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

void new_connection(int sockfd, const sockaddr_in& peer_addr)
{
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &peer_addr.sin_addr, ip, INET_ADDRSTRLEN);
  printf("newConnection(): accepted a new connection from %s:%d\n",
         ip, ntohs(peer_addr.sin_port));
  ::write(sockfd, "How are you?\n", 13);
  ::close(sockfd);
}

int main()
{
  printf("main(): pid = %d\n", getpid());
  sockaddr_in listen_addr;
  memset(&listen_addr, 0, sizeof(listen_addr));
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_addr.s_addr = INADDR_ANY;
  listen_addr.sin_port = htons(9981);
  Event_Loop loop;
  Acceptor acceptor(&loop, listen_addr);
  acceptor.set_new_connection_callback(new_connection);
  acceptor.listen();
  loop.loop();
}
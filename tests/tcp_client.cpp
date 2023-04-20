#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(9981);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    char hello[] = "Hello";
    write(sockfd, hello, strlen(hello));
    close(sockfd);
    return 0;
}

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define ERR_EXIT(m)                                                            \
  do {                                                                         \
    perror(m);                                                                 \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define MAXLINE 4096
#define SERV_PORT 9877
#define LISTENQ 1024
#define OPEN_MAX 1024
#define INFTIM -1

int main() {
  int listenfd, connfd, sockfd;
  if ((listenfd =
           socket(AF_INET, SOCK_STREAM, 0)) < 0)
    ERR_EXIT("socket error");

  struct sockaddr_in servaddr, cliaddr;

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(SERV_PORT);

  if (bind(listenfd, reinterpret_cast <sockaddr*>(&servaddr), sizeof(servaddr)) < 0)
    ERR_EXIT("bind error");

  if (listen(listenfd, LISTENQ) < 0)
    ERR_EXIT("listen error");

  int i, maxi, nready;
  socklen_t clilen;
  struct pollfd client[OPEN_MAX];

  client[0].fd = listenfd;
  client[0].events = POLLIN;
  for (i = 1; i < OPEN_MAX; ++i)
    client[i].fd = -1;
  maxi = 0;

  ssize_t n;
  char buf[MAXLINE];
  while (1) {
    if ((nready = poll(client, maxi + 1, INFTIM)) < 0)
      ERR_EXIT("poll error");

    if (client[0].revents & POLLIN) { /* new client connect */
      clilen = sizeof(cliaddr);
      if ((connfd = accept(listenfd, reinterpret_cast <sockaddr*>(&cliaddr), &clilen)) < 0)
        ERR_EXIT("accept error");

      printf("new client: %s, port %d\n", inet_ntoa(cliaddr.sin_addr),
             ntohs(cliaddr.sin_port));

      for (i = 1; i < OPEN_MAX; ++i)
        if (client[i].fd < 0) {
          client[i].fd = connfd;
          break;
        }

      if (i == OPEN_MAX)
        ERR_EXIT("too many clients");

      client[i].events = POLLIN;

      if (i > maxi)
        maxi = i;

      if (--nready <= 0)
        continue;
    }

    for (i = 1; i <= maxi; ++i) { /* check all clients for data */
      if ((sockfd = client[i].fd) < 0)
        continue;
      if (client[i].revents & (POLLIN | POLLERR)) {
        if ((n = read(sockfd, buf, MAXLINE)) < 0) {
          if (errno == ECONNRESET) {
            printf("client[%d] aborted connection\n", i);
            close(sockfd);
            client[i].fd = -1;
          } else
            ERR_EXIT("read error");
        } else if (n == 0) {
          printf("client[%d] closed connection\n", i);
          close(sockfd);
          client[i].fd = -1;
        } else {
          printf("clint[%d] send message: %s\n", i, buf);
          if (write(sockfd, buf, n) != n) {
            printf("write error \n");
            break;
          }
        }
        if (--nready <= 0)
          break;
      }
    }
  }
}
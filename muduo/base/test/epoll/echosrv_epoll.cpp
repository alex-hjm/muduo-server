
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
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
#define EPOLL_SIZE 50

void set_nonblock(int sockfd) { /*设置非阻塞边缘触发*/
  int flag;
  flag = fcntl(sockfd, F_GETFL, 0);
  if (flag < 0) {
    printf("fcntl get error \n");
    return;
  } // if

  if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {
    printf("fcntl set error \n");
    return;
  } // if
}

int main() 
{
  int listenfd, connfd, sockfd, epfd;
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    ERR_EXIT("socket error");
  
  set_nonblock(listenfd);

  struct sockaddr_in servaddr, cliaddr;

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(SERV_PORT);

  if (bind(listenfd, reinterpret_cast <sockaddr*>(&servaddr), sizeof(servaddr)) < 0)
    ERR_EXIT("bind error");

  if (listen(listenfd, LISTENQ) < 0)
    ERR_EXIT("listen error");

  struct epoll_event ev, events[EPOLL_SIZE];

  if ((epfd = epoll_create(EPOLL_SIZE)) < 0)
    ERR_EXIT("epoll_create error");

  ev.data.fd = listenfd;
  ev.events = EPOLLIN | EPOLLET;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) < 0)
    ERR_EXIT("epoll_ctl error");

  int i, nfds;
  socklen_t clilen;
  ssize_t n;
  char buf[MAXLINE];

  while (1) {
    if ((nfds = epoll_wait(epfd, events, EPOLL_SIZE, -1)) < 0)
      ERR_EXIT("epoll_wait error");

    for (i = 0; i < nfds; ++i) {
      if (events[i].data.fd == listenfd) { /* new client connection */
        clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, reinterpret_cast <sockaddr*>(&cliaddr), &clilen)) < 0)
          ERR_EXIT("eaccept error");

        printf("new client: %s, port %d\n", inet_ntoa(cliaddr.sin_addr),
               ntohs(cliaddr.sin_port));

        set_nonblock(connfd);
        ev.data.fd = connfd;
        ev.events = EPOLLIN | EPOLLET;

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev) < 0) 
          ERR_EXIT("epoll_ctl error");

      } else if (events[i].events & EPOLLIN) { /* new data in */
        if ((sockfd = events[i].data.fd) < 0)
          continue;
        if ((n = read(sockfd, buf, MAXLINE)) < 0) {
          if (errno == ECONNRESET) { 
            printf("client[%d] aborted connection\n", i);
            close(sockfd);
            events[i].data.fd = -1;
            } else 
             ERR_EXIT("read error");
        } else if (n == 0) {                  
          printf("client[%d] closed connection\n", i);
          close(sockfd);
          events[i].data.fd = -1;
        } else {
          printf("clint[%d] send message: %s\n", i, buf);
          ev.data.fd = sockfd; 
          ev.events = EPOLLOUT | EPOLLET;
          
          if (epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev) < 0) 
            ERR_EXIT("epoll_ctl error");
        }
      } else if (events[i].events & EPOLLOUT) { /* new data out */
        if ((sockfd = events[i].data.fd) < 0)
          continue;
        if (write(sockfd, buf, n) != n) {
          printf("write error \n");
          break;
        }                    
        ev.data.fd = sockfd; 
        ev.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev) < 0) 
          ERR_EXIT("epoll_ctl error");
      }
    }
  }
}
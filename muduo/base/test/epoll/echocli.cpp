#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define ERR_EXIT(m)                                                            \
  do {                                                                         \
    perror(m);                                                                 \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define MAXLINE 4096
#define SERV_PORT 9877

size_t readline(int fd, char *vptr, size_t maxlen) 
{
  size_t n, rc;
  char c, *ptr;

  ptr = vptr;
  for (n = 1; n < maxlen; n++) {
    if ((rc = read(fd, &c, 1)) == 1) {
      *ptr++ = c;
      if (c == '\n') break; /* newline is stored, like fgets() */
    } else if (rc == 0) {
      *ptr = 0;
      return (n - 1); /* EOF, n - 1 bytes were read */
    } else
      return (-1); /* error, errno set by read() */
  }

  *ptr = 0; /* null terminate like fgets() */
  return (n);
}

void str_cli(int sockfd) 
{
	char sendline[MAXLINE], recvline[MAXLINE];
	while (fgets(sendline, MAXLINE, stdin) != NULL) {
    if(static_cast<size_t>(write(sockfd, sendline, strlen(sendline)))!=strlen(sendline))
			ERR_EXIT("write over");

		//if(read(sockfd,recvline,MAXLINE)<0)
		if(readline(sockfd,recvline,MAXLINE) == 0)
			ERR_EXIT("server terminated prematurely");

		if(fputs(recvline, stdout) == EOF) 
			ERR_EXIT("fputs over");
	}
}

int main(int argc, char *argv[]) 
{
	if(argc !=2) 
		ERR_EXIT("usage: echocli <IPaddress>");

	int sockfd;
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
		ERR_EXIT("socket error");
	
	struct sockaddr_in servaddr;

	bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERV_PORT);
  servaddr.sin_port = htons(SERV_PORT);
	if(inet_aton(argv[1],&servaddr.sin_addr)<0)
		ERR_EXIT("inet_aton error");

	if(connect(sockfd, reinterpret_cast <sockaddr*>(&servaddr),sizeof(servaddr))<0)
		ERR_EXIT("connect error");

	str_cli(sockfd);

	close(sockfd);
}


#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: %s <port> <buffer_size>\n", argv[0]);
    exit(1);
  }

  int port = atoi(argv[1]);
  int bufsize = atoi(argv[2]);
  char *mesg = malloc(bufsize);
  if (!mesg) {
    perror("malloc");
    exit(1);
  }

  int sockfd, n;
  char ipadr[16];
  struct sockaddr_in servaddr, cliaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    free(mesg);
    exit(1);
  }

  memset(&servaddr, 0, SLEN);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(sockfd, (SADDR *)&servaddr, SLEN) < 0) {
    perror("bind problem");
    free(mesg);
    exit(1);
  }
  printf("UDP server is listening on port %d...\n", port);

  while (1) {
    unsigned int len = SLEN;

    if ((n = recvfrom(sockfd, mesg, bufsize, 0, (SADDR *)&cliaddr, &len)) < 0) {
      perror("recvfrom");
      free(mesg);
      exit(1);
    }
    mesg[n] = 0;

    printf("REQUEST: %s FROM %s:%d\n", mesg,
           inet_ntop(AF_INET, (void *)&cliaddr.sin_addr.s_addr, ipadr, 16),
           ntohs(cliaddr.sin_port));

    if (sendto(sockfd, mesg, n, 0, (SADDR *)&cliaddr, len) < 0) {
      perror("sendto");
      free(mesg);
      exit(1);
    }
  }

  free(mesg);
}

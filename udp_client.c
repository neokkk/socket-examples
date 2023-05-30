#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "custom_error.c"

#define BUF_SIZE 1024

int main(int argc, char** argv) {
  struct sockaddr_in client_addr, server_addr;
  int sockfd;
  int bytesread, addrlen;
  char buf[BUF_SIZE];

  if (argc < 3) {
    print_error("Host and port is not specified");
    exit(1);
  }

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    print_error("Client socket() failed");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(argv[1]); // 호스트명이 아닌 IP 주소를 입력해야 한다.
  server_addr.sin_port = htons(atoi(argv[2]));

  while (1) {
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
      printf("EOF\n");
      break;
    }

    if (sendto(sockfd, buf, strlen(buf), MSG_WAITALL, (struct sockaddr*)&server_addr, sizeof server_addr) < 0) {
      print_error("Client sendto() failed");
      exit(1);
    }

    addrlen = sizeof(client_addr);
    if ((bytesread = recvfrom(sockfd, buf, sizeof(buf), MSG_WAITALL, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen)) < 0) {
      print_error("Client recvfrom() failed");
      exit(1);
    }

    buf[bytesread] = '\0';
    printf("Server says(%d bytes): %s\n", bytesread, buf);
  }

  if (close(sockfd) < 0) {
    print_error("Client close() failed");
    exit(1);
  }

  return 0;
}
#include <arpa/inet.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "custom_error.c"

#define BUF_SIZE 1024

int main(int argc, char** argv) {
  struct sockaddr_in server_addr, client_addr;
  int sockfd;
  int bytesread, addrlen;
  char buf[BUF_SIZE];

  if (argc != 2) {
    print_error("Port is not specified");
    exit(1);
  }

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    print_error("Server socket() failed");
    exit(1);
  }

  memset((void*)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(atoi(argv[1]));

  if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    print_error("Server bind() failed");
    exit(1);
  }

  printf("Server is waiting on %d\n", ntohs(server_addr.sin_port));

  while (1) {
    for (;;) {
      addrlen = sizeof(client_addr);
      if ((bytesread = recvfrom(sockfd, buf, sizeof(buf) - 1, MSG_WAITALL, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen)) < 0) {
        print_error("Server recvfrom() failed");
        exit(1);
      }

      buf[bytesread] = '\0';
      printf("Client says(%d bytes): %s\n", bytesread, buf);

      for (int i = 0; i < bytesread; i++) {
        buf[i] = toupper(buf[i]);
      }

      if (sendto(sockfd, buf, bytesread, 0, (struct sockaddr*)&client_addr, (socklen_t)addrlen) != bytesread) {
        print_error("Server send() failed");
        exit(1);
      }
    }
  }

  if (close(sockfd) < 0) {
    print_error("Server close() failed");
    exit(1);
  }

  return 0;
}
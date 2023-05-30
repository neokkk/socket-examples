#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "custom_error.c"

#define BUF_SIZE 1024
#define MAX_FD_SIZE FD_SETSIZE - 1

int main(int argc, char** argv) {
  struct sockaddr_in server_addr, client_addr;
  int sockfd;
  int fd_max;
  fd_set set, rset;
  char buf[BUF_SIZE];

  if (argc != 2) {
    print_error("Port is not specified");
    exit(1);
  }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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

  if (listen(sockfd, MAX_FD_SIZE) < 0) {
    print_error("Server listen() failed");
    exit(1);
  }

  printf("Server is listening on %d\n", ntohs(server_addr.sin_port));

  FD_ZERO(&set);
  FD_SET(sockfd, &set);

  fd_max = sockfd;

  while (1) {
    rset = set;
    int fd_num = select(fd_max + 1, &rset, NULL, NULL, NULL);

    if (fd_num < 0) {
      print_error("Server select() failed");
      exit(1);
    }

    if (fd_num == 0) continue;

    for (int i = sockfd; i <= fd_max; i++) {
      if (!FD_ISSET(i, &rset)) continue;
      if (i == sockfd) {
        int addrlen = sizeof(client_addr);
        int new_sockfd;
        char welcome_message[50];
        char ip_address[20];

        if ((new_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen)) < 0) {
          if (errno == EINTR) continue;
          print_error("Server accpet() failed");
          exit(1);
          break;
        }

        FD_SET(new_sockfd, &set);
        if (fd_max < new_sockfd) fd_max = new_sockfd;
        printf("Client connected %s (%d)\n", inet_ntoa(client_addr.sin_addr), new_sockfd);
        sprintf(welcome_message, "Welcome %s:%d!\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        send(new_sockfd, welcome_message, strlen(welcome_message), 0);
      } else {
        memset(buf, 0, sizeof(buf));

        int bytes_recv = recv(i, buf, sizeof(buf) - 1, 0);
        if (bytes_recv < 0) {
          print_error("Server recv() failed");
          exit(1);
        }

        if (bytes_recv == 0) {
          printf("Client %d disconnected\n");
          FD_CLR(i, &set);
          if (close(i) < 0) {
            print_error("Server close() failed");
            exit(1);
          }
          break;
        }

        buf[bytes_recv] = '\0';
        printf("%s", buf);

        for (int j = sockfd + 1; j <= fd_max; j++) { // broadcasting
          if (i == j) continue;
          if (send(j, buf, sizeof(buf), 0) < 0) {
            print_error("Server send() broadcast failed");
            exit(1);
          }
        }
      }
    }
  }

  if (close(sockfd) < 0) {
    print_error("Server close() failed");
    exit(1);
  }

  return 0;
}
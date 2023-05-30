#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "custom_error.c"

#define BUF_SIZE 1024

int get_max_fd(int* fds, int count) {
  int min_idx = -1;
  for (int i = 1; i <= count; i++) {
    if (fds[i] < 0) {
      min_idx = i;
      break;
    }
  }
  return min_idx;
}

int main(int argc, char** argv) {
  struct sockaddr_in server_addr, client_addr;
  int addrlen;
  int flags;
  int sockfd;
  fd_set set, rset;
  int fd_max, fd_num;

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

  if (listen(sockfd, SOMAXCONN) < 0) {
    print_error("Server listen() failed");
    exit(1);
  }

  printf("Server is listening on %d\n", ntohs(server_addr.sin_port));

  // 논블로킹 소켓으로 전환
  flags = fcntl(sockfd, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(sockfd, F_SETFL, flags);

  FD_ZERO(&set);
  FD_SET(sockfd, &set);
  fd_max = sockfd;

  while (1) {
    rset = set; // 원본이 변경되므로 복사본 사용
    fd_num = select(fd_max + 1, &rset, NULL, NULL, NULL);

    if (fd_num < 0) {
      print_error("Server select() failed");
      exit(1);
      break;
    }

    if (fd_num == 0) continue; // 타임아웃

    for (int i = 0; i <= fd_max; i++) {
      if (FD_ISSET(i, &rset)) { // 소켓 디스크립터에 변경이 있으면
        if (i == sockfd) { // 서버 소켓
          addrlen = sizeof(client_addr);
          int new_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen);
          if (new_sockfd < 0) {
            print_error("Server accept() failed");
            exit(1);
          }

          printf("Client connected from %d\n", ntohs(client_addr.sin_port));
          FD_SET(new_sockfd, &set);
          if (fd_max < new_sockfd) fd_max = new_sockfd;
        } else { // 클라이언트 소켓
          printf("[Client %d] ", i);
          char buf[BUF_SIZE];
          int bytes_recv = recv(i, buf, sizeof(buf) - 1, 0);
          printf("bytes_recv: %d\n", bytes_recv);
          if (bytes_recv < 0) {
            print_error("Server recv() failed");
            exit(1);
          }
          if (bytes_recv == 0) {
            FD_CLR(i, &rset);
            if (close(i) < 0) {
              print_error("Server close() failed");
              exit(1);
            }
          } else {
            printf("%s", buf);
            if (write(i, buf, bytes_recv) != bytes_recv) { // echo
              print_error("Server write() failed");
              exit(1);
            }
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
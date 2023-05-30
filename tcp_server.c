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
  char buf[BUF_SIZE];
  int sockfd, new_sockfd;
  int bytesread, addrlen;

  if (argc != 2) {
    print_error("Port number is not specified");
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

  while (1) { // 루프를 돌면서 클라이언트 접속 대기
    addrlen = sizeof(client_addr);
    if ((new_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen)) < 0) {
      if (errno == EINTR) continue; // 시그널 발생으로 인한 오류일 경우 재시도
      print_error("accept() failed");
      exit(1);
    }
    printf("Client is connected: %s:%d, on socket %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), new_sockfd);

    for (;;) {
      if ((bytesread = recv(new_sockfd, buf, sizeof(buf) - 1, MSG_WAITALL)) < 0) {
        print_error("Server recv() failed");
        exit(1);
      }

      if (bytesread == 0) {
        printf("Client is disconnected\n");
        close(new_sockfd);
        break;
      }

      buf[bytesread] = '\0';
      printf("Client says(%d bytes): %s\n", bytesread, buf);

      // 버퍼 데이터 처리하기
      for (int i = 0; i < bytesread; i++) {
        buf[i] = toupper(buf[i]);
      }

      if (send(new_sockfd, buf, bytesread, 0) != bytesread) {
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
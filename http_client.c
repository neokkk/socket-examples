#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "custom_error.c"

#define BUF_SIZE 1024

int connect_server(char* hostname, unsigned int port) {
  struct sockaddr_in server_addr;
  struct hostent* hostp;
  int sockfd;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    print_error("Client socket() failed");
    exit(1);
  }

  if ((hostp = gethostbyname(hostname)) < 0) {
    print_error("Client gethostbyname() failed");
    exit(1);
  }

  memset((void*)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  memcpy((void*)&server_addr.sin_addr, hostp ->h_addr_list[0], hostp ->h_length);
  server_addr.sin_port = htons(port);
  printf("Connected to %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

  if (connect(sockfd, (struct sockaddr*)&server_addr, (socklen_t)sizeof(server_addr)) < 0) {
    print_error("Client connect() failed");
    exit(1);
  }

  return sockfd;
}

void request_file(int fd, char* hostname, char* filepath) {
  char buf[BUF_SIZE];
  int bytes_send;

  memset(buf, 0, sizeof(buf));
  sprintf(buf, "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: close\r\n\r\n", filepath, hostname, "webcli/1.0");
  printf("%s\n", buf);

  if ((bytes_send = send(fd, buf, strlen(buf), 0)) < 0) {
    print_error("Client send() failed");
    exit(1);
  }
}

void parse_response(int fd, char* filepath) {
  char* filename = strrchr(filepath, '/');
  char buf[BUF_SIZE];
  FILE* fp = fdopen(fd, "r");

  if (filename) { // 서버 리소스 경로 제거
    filename += 1;
  } else {
    filename = filepath;
  }

  while (1) { // 헤더 읽기
    char* last = fgets(buf, sizeof(buf), fp);
    if (!last) {
      print_error("Client fgets() failed");
      exit(1);
    }

    if (*last == '\n' || *last == '\r') {
      break;
    }

    char* header = strtok(buf, "\n");
    int status_code;
    long content_len;

    printf("%s\n", header);

    if (strncmp(header, "HTTP/", 5) == 0) {
      status_code = atoi(strchr(header, '/') + 5);
      printf("Status-Code: %d\n", status_code);
    }

    if (status_code >= 500) {
      print_error("Server connection failed");
      exit(1);
    }

    if (strncmp(header, "Content-Length", 14) == 0) {
      content_len = atol(strchr(header, ':') + 2);
      printf("Total: %ld (bytes)\n", content_len);
    }
  }

  FILE* new_fp = fopen(filename, "w+");

  while (1) { // 데이터(바디) 읽기
    int bytes_read;
    if ((bytes_read = fread(buf, sizeof(char), sizeof(buf), fp)) < 0) {
      print_error("Client fread() failed");
      exit(1);
    }

    if (bytes_read == 0) break;

    if (fwrite(buf, sizeof(char), bytes_read, new_fp) < 0) {
      print_error("Client fwrite() failed");
      exit(1);
    }
  }
}

int main(int argc, char** argv) {
  int sockfd;
  int bytes_send, bytes_recv;
  char buf[BUF_SIZE];

  if (argc != 4) {
    print_error("Host, port, filename are not specified");
    exit(1);    
  }

  sockfd = connect_server(argv[1], atoi(argv[2]));
  request_file(sockfd, argv[1], argv[3]);
  parse_response(sockfd, argv[3]);

  return 0;
}
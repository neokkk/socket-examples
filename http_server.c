#include <assert.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "custom_error.c"

#define BUF_SIZE 1024

int create_server(int port) {
  struct sockaddr_in server_addr, client_addr;
  int sockfd;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    print_error("Server socket() failed");
    exit(1);
  }

  memset((void*)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr*)&server_addr, (socklen_t)sizeof(server_addr)) < 0) {
    print_error("Server bind() failed");
    exit(1);
  }

  if (listen(sockfd, SOMAXCONN) < 0) {
    print_error("Server listen() failed");
    exit(1);
  }

  printf("Server is listening on %d\n", ntohs(server_addr.sin_port));

accept:
  while (1) {
    int new_sockfd;
    socklen_t addrlen = sizeof(client_addr);
    FILE* new_fp;
    char *filepath, *ext;
    char buf[BUF_SIZE];

    memset(buf, 0, sizeof(buf));

    if ((new_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen)) < 0) {
      print_error("Server accept() failed");
      exit(1);
    }

    if (!(new_fp = fdopen(new_sockfd, "r"))) {
      print_error("Server fdopen failed");
      exit(1);
    }

    for (;;) { // 요청 헤더 읽기
      char *line;

      if (!(line = fgets(buf, sizeof(buf), new_fp))) {
        print_error("Server fgets() failed");
        fclose(new_fp);
        if (close(new_sockfd)) {
          print_error("Server close() failed");
        }
        goto accept;
      }

      if (*line == '\n' || *line == '\r') break;
      printf("%s", line);

      if (strncmp(buf, "GET", 3) == 0) {
        char* tmp_method = strtok(buf, " \t\r\n");
        char* tmp_filepath = strtok(NULL, " \t\r\n") + 1;
        char* tmp_ext = strrchr(tmp_filepath, '.') + 1;

        filepath = (char*)malloc(strlen(tmp_filepath) + 1);
        strcpy(filepath, tmp_filepath);

        ext = (char*)malloc(strlen(tmp_ext) + 1);
        strcpy(ext, tmp_ext);

        if (!ext) {
          print_error("Invalid file extension");
          fclose(new_fp);
          close(new_sockfd);
          exit(1);
        }
      }
    }

    FILE* fp;
    struct stat statinfo;
    char* status_code = "200 OK";
    char* content_type = "text/plain";
    unsigned long filesize, total_read;
    char header_buf[BUF_SIZE];
    char body_buf[BUF_SIZE];

    memset(header_buf, 0, sizeof(header_buf));
    memset(body_buf, 0, sizeof(body_buf));

    if (stat(filepath, &statinfo) < 0) { // 요청 파일이 존재하지 않을 때
      print_error("Cannot find resource");
      strcat(header_buf, "HTTP/1.1 404 NOT FOUND\r\n");
      strcat(header_buf, "Connection: close\r\n");
      strcat(header_buf, "Content-Length: 0\r\n");
      strcat(header_buf, "Content-Type: text/html\r\n\r\n");
      strcat(header_buf, "<html><body>File not found</body></html>\r\n");

      if (send(new_sockfd, header_buf, strlen(header_buf), 0) < 0) {
        print_error("Server send() failed");
        exit(1);
      }
    } else {
      filesize = total_read = 0;
      filesize = statinfo.st_size;
      printf("filesize: %ld\n", filesize);

      if (!(fp = fopen(filepath, "rb"))) {
        print_error("Server fopen() failed");
        exit(1);
      }

      if (strcmp("html", ext) == 0) {
        content_type = "text/html";
      } else if (strcmp("jpg", ext) == 0) {
        content_type = "image/jpeg";
      }

      sprintf(header_buf, "HTTP/1.1 %s\r\nConnection: close\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n", status_code, statinfo.st_size, content_type);

      if (send(new_sockfd, header_buf, strlen(header_buf), 0) < 0) { // 응답 헤더 전송
        print_error("Server send() failed");
        exit(1);
      }

      for (;;) { // 응답 데이터(바디) 전송
        int bytes_read;
        if ((bytes_read = fread(body_buf, sizeof(char), sizeof(body_buf) - 1, fp)) < 0) {
          print_error("Server fread() failed");
          exit(1);
        }

        if (bytes_read == 0) break;

        total_read += bytes_read;
        // printf("%ld%% downloaded...\n", (long)(1.0 * total_read / filesize * 100));

        if (send(new_sockfd, body_buf, bytes_read, 0) < 0) {
          print_error("Server send() failed");
          exit(1);
        }
      }
    }

    free(filepath);
    free(ext);
    fclose(new_fp);
    fclose(fp);
  }

  return sockfd;
}

int main(int argc, char** argv) {
  int sockfd;

  if (argc != 2) {
    print_error("Port is not specified");
    exit(1);    
  }

  sockfd = create_server(atoi(argv[1]));

  close(sockfd);
}
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "custom_error.c"

#define BUF_SIZE 1024

int main(int argc, char** argv) {
  struct sockaddr_in server_addr;
  int sockfd;
  int bytesread;
  struct hostent* hostp;
  char buf[BUF_SIZE];

  if (argc != 3) {
    print_error("Host and port is not specified");
    exit(1);
  }

  if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    print_error("Client socket() failed");
    exit(1);
  }

  if ((hostp = gethostbyname(argv[1])) < 0) {
    print_error("Cannot get hostname of server");
    exit(1);
  }

  memset((void*)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  memcpy((void*)&server_addr.sin_addr, hostp ->h_addr_list[0], hostp ->h_length);
  server_addr.sin_port = htons(atoi(argv[2]));

  if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    print_error("Client connect() failed");
    exit(1);
  }

  while (1) {
    if (fgets(buf, sizeof(buf), stdin) == NULL) {
      printf("EOF\n");
      break;
    }

    if (send(sockfd, buf, strlen(buf), 0) < 0) {
      print_error("Client send() failed");
      exit(1);
    }

    if ((bytesread = recv(sockfd, buf, strlen(buf), 0)) < 0) {
      print_error("Client recv() failed");
      exit(1);
    }

    buf[bytesread] = '\0';
    printf("Server says(%d bytes): %s\n", bytesread, buf);
  }

  if (close(sockfd)) {
    print_error("Client close() failed");
    exit(1);
  }

  free(hostp);
  return 0;
}
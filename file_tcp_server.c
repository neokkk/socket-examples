#include <arpa/inet.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "custom_error.c"
#include "file.c"

#define BUF_SIZE 1024
#define MAX_FD_SIZE FD_SETSIZE - 1

void load_file(int new_sockfd, char* filename) {
  printf("filename: %s\n", filename);
  struct stat statbuf;
  char buf[BUF_SIZE];
  char msg[50];

  if (strlen(filename) == 0) {
    print_error("No filename");
    return;
  }

  if (stat(filename, &statbuf) < 0) {
    print_error("Server stat() failed");
    return;
  }

  if (statbuf.st_size >= 0) {
    printf("File size: %dbytes\n", statbuf.st_size);
    int bytes_total = statbuf.st_size;
    FILE* fp;

    if (!(fp = fopen(filename, "r"))) {
      print_error("Server fopen() failed");
      return;
    }

    memset(buf, 0, sizeof(buf));

    while (fread(buf, sizeof(char), sizeof(buf), fp) > 0) {
      if (send(new_sockfd, buf, sizeof(buf), 0) < 0) {
        print_error("Server send() failed");
        fclose(fp);
        return;
      }
    }

    fclose(fp);
  }
}

void store_file(int new_sockfd, char* filename) {
  char buf[BUF_SIZE];
	int num = 0;
	FILE* fp;

	if (!(fp = fopen(filename, "w"))) {
    print_error("Server fopen() failed");
    return;
  }

  memset(buf, 0, sizeof(buf));
 
	while((num = fread(buf, sizeof(char), sizeof(buf), fp)) > 0) {
		buf[num] = 0;
		if (fwrite(buf, sizeof(char), num, fp) != num) {
      print_error("Server fwrite() failed");
			fclose(fp);
			return;
		}
	}

	fclose(fp);
}

void print_file(int new_sockfd, char* extension) {
  DIR* dirp;
  struct dirent* dirent;
  char buf[BUF_SIZE];
  int fcount = 0;

  if (strlen(extension) == 0) {
    printf("No extension");
    return;
  }

  memset(buf, 0, sizeof(buf));

  if (!(dirp = opendir("."))) {
    print_error("Server opendir() failed");
    return;
  }

  while (dirent = readdir(dirp)) {
    char* filename = dirent ->d_name;
    char* ext = strrchr(filename, '.');

    if (!ext) continue;
    if (strcmp(extension, ext + 1) == 0) {
      strcat(buf, filename);
      strcat(buf, "\n");
      fcount++;
    }
  }

  if (fcount > 0) { 
    printf("%s", buf);
  } else {
    printf("File cannot found");
    sprintf(buf, "%s\n", "File cannot found");
  }

  if (send(new_sockfd, buf, sizeof(buf), 0) < 0) {
    print_error("Server send() failed");
    return;
  }
}

int main(int argc, char** argv) {
  struct sockaddr_in server_addr, client_addr;
  fd_set rset, rset_copied;
  int sockfd, fd_max;
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

  FD_ZERO(&rset);
  FD_SET(sockfd, &rset);
  FD_SET(1, &rset);

  fd_max = sockfd;

  while (1) {
    rset_copied = rset;
    int fd_num = select(fd_max + 1, &rset_copied, NULL, NULL, NULL);

    if (fd_num < 0) {
      print_error("Server select() failed");
      exit(1);
    }

    if (fd_num == 0) continue;

    for (int i = sockfd; i <= fd_max; i++) {
      if (!FD_ISSET(i, &rset_copied)) continue;
      if (i == sockfd) {
        int new_sockfd;
        int addrlen = sizeof(client_addr);

        if ((new_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen)) < 0) {
          if (errno == EINTR) continue;
          print_error("Server accept() failed");
          exit(1);
          break;
        }

        FD_SET(new_sockfd, &rset);
        printf("Client connected %s (%d)\n", inet_ntoa(client_addr.sin_addr), new_sockfd);
        if (fd_max < new_sockfd) fd_max = new_sockfd;
      } else {
        FILE* fp = fdopen(i, "r");

        if (!fgets(buf, sizeof(buf), fp)) {
          print_error("File gets() failed");
          fclose(fp);
          FD_CLR(i, &rset);

          if (close(i) < 0) {
            print_error("Server close() failed");
            exit(1);
          }
          break;
        }

        fflush(stdout);
        char* command = strtok(buf, " ");
        char* option = strtok(NULL, " ");
        option[strlen(option) - 1] = '\0';
        printf("%s\n", command);
        printf("%s\n", option);

        if (strcmp(command, "GET") == 0) {
          load_file(i, option);
        } else if (strcmp(command, "LS") == 0) {
          print_file(i, option);
        } else if (strcmp(command, "PUT") == 0 ) {
          store_file(i, option);
        } else {
          printf("Unknown command %s\n", command);
          continue;
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
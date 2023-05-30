#include <errno.h>
#include <stdio.h>
#include <string.h>

void print_error(char* msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}
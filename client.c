#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "tftp.h"

int client_fd;

int graceful_stop(int signal) {
  close(client_fd);
  exit(EXIT_SUCCESS);
}

int mkdir(int argc, char **argv);
int put(int argc, char **argv);
int get(int argc, char **argv);
int pwd(int argc, char **argv);
int cwd(int argc, char **argv);
int delete(int argc, char **argv);

static int set_args(char *args, char **argv) {
  int count = 0;

  while (isspace(*args))
    ++args;
  while (*args) {
    if (argv)
      argv[count] = args;
    while (*args && !isspace(*args))
      ++args;
    if (argv && *args)
      *args++ = '\0';
    while (isspace(*args))
      ++args;
    count++;
  }
  return count;
}

char **parsed_args(char *args, int *argc) {
  char **argv = NULL;
  int argn = 0;

  if (args && *args && (args = strdup(args)) && (argn = set_args(args, NULL)) &&
      (argv = malloc((argn + 1) * sizeof(char *)))) {
    *argv++ = args;
    argn = set_args(args, argv);
  }

  if (args && !argv)
    free(args);

  *argc = argn;
  return argv;
}

void free_parsed_args(char **argv) {
  if (argv) {
    free(argv[-1]);
    free(argv - 1);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  printf("args: %s:%s\n", argv[1], argv[2]);

  char *addr = argv[1];
  uint16_t port = atoi(argv[2]);
  int status;
  // char *buffer = malloc(BUFFER_SIZE);

  printf("config: %s:%d\n", addr, port);

  struct sockaddr_in serv_addr;

  if (0 > (client_fd = socket(AF_INET, SOCK_STREAM, 0))) {
    printf("\n Socket creation error\n");
    exit(EXIT_FAILURE);
  }
  printf("socket created\n");

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, addr, &serv_addr.sin_addr) <= 0) {
    printf("inet_pton error. invalid address\n");
    exit(EXIT_FAILURE);
  }

  if (0 > (status = connect(client_fd, (struct sockaddr *)&serv_addr,
                            sizeof(serv_addr)))) {
    printf("connection failed\n");
    exit(EXIT_FAILURE);
  }

  char **l_argv;
  int l_argc;

  while (1) {
    printf("tftp > ");
    char *input = malloc(128);
    fgets(input, 128, stdin);

    l_argv = parsed_args(input, &argc);

    if (strcmp(l_argv[0], "mkdir") == 0) {

    } else if (strcmp(l_argv[0], "get") == 0) {

    } else if (strcmp(l_argv[0], "put") == 0) {

    } else if (strcmp(l_argv[0], "cwd") == 0) {

    } else if (strcmp(l_argv[0], "pwd") == 0) {

    } else if (strcmp(l_argv[0], "delete") == 0) {
    }

    printf("input(%lu): %s\n", strlen(input), input);
    send(client_fd, input, strlen(input), 0);
  }

  graceful_stop(EXIT_FAILURE);
}

int mkdir(int argc, char **argv) { return EXIT_SUCCESS; }
int put(int argc, char **argv) { return EXIT_SUCCESS; }
int get(int argc, char **argv) { return EXIT_SUCCESS; }

int pwd(int argc, char **argv) { return EXIT_SUCCESS; }

int cwd(int argc, char **argv) { return EXIT_SUCCESS; }

int delete(int argc, char **argv) { return EXIT_SUCCESS; }

#include <arpa/inet.h>
#include <netinet/in.h>
// usage client.exe ip port
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE (32 << 20)

int client_fd;

int graceful_stop(int signal) {
  close(client_fd);
  exit(EXIT_SUCCESS);
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
  char *buffer = malloc(BUFFER_SIZE);

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

  char *input = malloc(BUFFER_SIZE);
  printf("ENTER MESSAGE: ");
  fgets(input, BUFFER_SIZE, stdin);
  printf("len of input: %lu\n", strlen(input));
  send(client_fd, buffer, strlen(input), 0);

  graceful_stop(EXIT_FAILURE);
}

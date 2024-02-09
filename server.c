#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE (32 << 20)

int sockfd;

int graceful_stop(int signal) {
  close(sockfd);
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("usage: %s <port> <path>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  fprintf(stdout, "argc: %d\n", argc);
  fprintf(stdout, "args: %s %s\n", argv[1], argv[2]);
  int port = atoi(argv[1]);
  printf("converted port: %d\n", port);
  // server attributes
  int new_socket;
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);
  ssize_t valread;
  char *buffer;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed\n");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (0 > bind(sockfd, (struct sockaddr *)&address, sizeof(address))) {
    perror("bind error\n");
    exit(EXIT_FAILURE);
  }

  if (0 > listen(sockfd, 3)) {
    perror("listen error\n");
    exit(EXIT_FAILURE);
  }

  printf("server started on port %d\n", port);

  while (1) {
    if (0 >
        (new_socket = accept(sockfd, (struct sockaddr *)&address, &addrlen))) {
      perror("accept error");
      exit(EXIT_FAILURE);
    }

    if (fork() == 0) {
      char buffer_ip[16] = {0};
      inet_ntop(AF_INET, &address, buffer_ip, addrlen);
      printf("Incoming connection from %s\n", buffer_ip);

      buffer = malloc(BUFFER_SIZE);

      ssize_t len = read(new_socket, buffer, BUFFER_SIZE - 1);

      buffer[len] = 0;

      printf("DATA RECIEVED from %s: %s\n\n", buffer_ip, buffer);

      free(buffer);
      close(new_socket);
    }
  }

  graceful_stop(EXIT_SUCCESS);
}

#include <arpa/inet.h>
#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "tftp.h"

int sockfd;
struct sockaddr_in saddr_other;
socklen_t addrlen = sizeof(saddr_other);

int graceful_stop(int signal) {
  close(sockfd);
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

  // printf("args: %s:%s\n", argv[1], argv[2]);

  char *addr = argv[1];
  uint16_t port = atoi(argv[2]);

  if (0 > (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) {
    printf("\n Socket creation error\n");
    exit(EXIT_FAILURE);
  }
  memset((char *)&saddr_other, 0, sizeof(saddr_other));
  saddr_other.sin_family = AF_INET;
  saddr_other.sin_port = htons(port);

  if (inet_aton(addr, &saddr_other.sin_addr) == 0) {
    printf("inet_aton error. invalid address\n");
    exit(EXIT_FAILURE);
  }

  // if (0 > (status = connect(client_fd, (struct sockaddr *)&serv_addr,
  //                           sizeof(serv_addr)))) {
  //   printf("connection failed\n");
  //   exit(EXIT_FAILURE);
  // }

  char **l_argv;
  int l_argc;

  while (1) {
    printf("unreal-tftp > ");
    char *input = malloc(128);
    fgets(input, 128, stdin);

    l_argv = parsed_args(input, &argc);

    if (strcmp(l_argv[0], "mkdir") == 0) {

    } else if (strcmp(l_argv[0], "get") == 0) {

    } else if (strcmp(l_argv[0], "put") == 0) {
      if (EXIT_SUCCESS != put(l_argc, l_argv)) {
        printf("put error\n");
      }
    } else if (strcmp(l_argv[0], "cwd") == 0) {

    } else if (strcmp(l_argv[0], "pwd") == 0) {

    } else if (strcmp(l_argv[0], "delete") == 0) {
    }
  }
  graceful_stop(EXIT_FAILURE);
}

// argv[1] - filename;
int put(int argc, char **argv) {

  int fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    printf("Error! File cannot be opened.\n");
    return EXIT_FAILURE;
  }
  off_t fsize = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  // tftp_data_t data;
  tftp_wrrq_t req;

  // op_code
  req.op_code = TFTP_WRQ;

  // file_name
  int file_name_length = strlen(argv[1]);
  req.file_name = malloc(file_name_length);
  strcpy(req.file_name, argv[1]);
  // printf("put: WRQ.file_name = %s\n", req.file_name);
  // fflush(stdout);

  // mode
  int mode_length = strlen("octet");
  req.mode = malloc(16);
  strcpy(req.mode, "octet");
  // printf("put: WRQ.mode = %s\n", req.mode);
  // fflush(stdout);

  // allocating RRQ
  char *packet = malloc(BLOCK_SIZE);
  char *p = packet;

  // preparing RRQ
  memcpy(p, &req.op_code, sizeof(req.op_code));
  p += 2;

  memcpy(p, req.file_name, file_name_length);
  p += file_name_length;
  *p++ = 0;

  memcpy(p, req.mode, mode_length);
  p += mode_length;
  *p = 0;

  printf("put: WRQ %d %s %s\n", req.op_code, req.file_name, req.mode);

  sendto(sockfd, packet, BLOCK_SIZE, 0, (struct sockaddr *)&saddr_other,
         addrlen);

  tftp_ack_t ack_packet;
  char recieved[BLOCK_SIZE];
  for (int written = 0; written < fsize; written += BLOCK_SIZE) {
    fflush(stdout);
    int recieved_bytes = recvfrom(sockfd, &recieved, BLOCK_SIZE, 0,
                                  (struct sockaddr *)&saddr_other, &addrlen);

    int op_code = 0;
    memcpy(&op_code, recieved, 2);

    // catching error
    if (op_code != TFTP_ACK) {
      tftp_err_t err_packet;
      memcpy(&err_packet, recieved, sizeof(tftp_err_t));
      err_packet.msg = recieved + 4;
      printf("Error packet recieved: [%d] %s\n", err_packet.err,
             err_packet.msg);
      graceful_stop(err_packet.err);
    }

    // unpacking acknowlegment packet
    memcpy(&ack_packet, recieved, sizeof(tftp_ack_t));

    printf("put: Recieved ACK#%d\n", ack_packet.block_n);

    printf("put: position of file - %ld\n",
           lseek(fd, (ack_packet.block_n) * BLOCK_SIZE, SEEK_SET));

    uint8_t *buffer = malloc(BLOCK_SIZE);

    // op_code = 0;
    // memcpy(&op_code, buffer, 2);

    // if (op_code != TFTP_DATA) {
    //   tftp_err_t err;
    //   memcpy(&err, buffer, sizeof(tftp_err_t));
    //   err.msg = buffer + 4;
    //   printf("Recieved error packet: [%d] %s", err.err, err.msg);
    //   return err.err;
    // }

    int to_write = read(fd, buffer, BLOCK_SIZE);
    if (to_write == -1) {
      printf("failed to read\n");
    }

    // preparing data packet
    tftp_data_t data_packet;
    data_packet.op_code = TFTP_DATA;
    data_packet.data = buffer;
    data_packet.block_n = ack_packet.block_n + 1;

    uint8_t *packet = malloc(4 + to_write);
    memcpy(packet, &data_packet, 4);
    memcpy(packet + 4, data_packet.data, to_write);

    sendto(sockfd, (char *)packet, 4 + to_write, 0,
           (struct sockaddr *)&saddr_other, addrlen);
    printf("put: sent %d bytes "
           "DATA#%d\n--------\n%s\n---------\n\n",
           4 + to_write, data_packet.block_n, data_packet.data);

    free(buffer);
  }

  int recieved_bytes = recvfrom(sockfd, &recieved, BLOCK_SIZE, 0,
                                (struct sockaddr *)&saddr_other, &addrlen);
  memcpy(&ack_packet, recieved, sizeof(tftp_ack_t));

  printf("put: Recieved ACK#%d\n", ack_packet.block_n);
  return EXIT_SUCCESS;
}
int get(int argc, char **argv) { return EXIT_SUCCESS; }

#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tftp.h"

int sockfd;
int root_dirfd;
struct sockaddr_in saddr_me, saddr_other;
socklen_t addrlen = sizeof(saddr_other);

int graceful_stop(int signal) {
  close(sockfd);
  close(root_dirfd);
  exit(EXIT_SUCCESS);
}

int cmd_put(char *buffer, int length, struct sockaddr_in saddr_other,
            socklen_t addrlen);
int cmd_get(char *buffer, int length, struct sockaddr_in saddr_other,
            socklen_t addrlen);
int main(int argc, char *argv[]) {
  int port = argc < 2 ? DEFAULT_PORT : atoi(argv[1]);
  printf("converted port: %d\n", port);

  char *root_dir = malloc(PATH_MAX);

  if (argc < 3) {
    strcpy(root_dir, "./docs");
  } else {
    strcpy(root_dir, argv[2]);
  }

  printf("root_dir = %s\n", root_dir);

  struct stat st = {0};
  if (stat(root_dir, &st) == -1) {
    char *cmd = malloc(PATH_MAX);

    sprintf(cmd, "mkdir %s -p", root_dir);
    system(cmd);

    free(cmd);
  }

  // printf("Opening root_dir = %s\n", root_dir);
  if (-1 == (root_dirfd = open(root_dir, O_RDONLY))) {
    printf("cannot open dirfd\n");
    graceful_stop(EXIT_FAILURE);
  };

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket failed\n");
    exit(EXIT_FAILURE);
  }

  saddr_me.sin_family = AF_INET;
  saddr_me.sin_addr.s_addr = INADDR_ANY;
  saddr_me.sin_port = htons(port);

  if (0 > bind(sockfd, (struct sockaddr *)&saddr_me, sizeof(saddr_me))) {
    perror("bind() error\n");
    exit(EXIT_FAILURE);
  }

  printf("server started on port %d\n", port);

  int recv_len;

  char *recieved = malloc(BLOCK_SIZE);
  while (1) {

    printf("\n\nWaiting for packets...\n");
    fflush(stdout);

    // if (fork() == 0) {
    // }

    if (-1 ==
        (recv_len = recvfrom(sockfd, recieved, 2 * BLOCK_SIZE, 0,
                             (struct sockaddr *)&saddr_other, &addrlen))) {
      graceful_stop(0);
    }

    uint16_t op_code = 0;
    memcpy(&op_code, recieved, 2);
    printf("Incoming connection from %s:%d OP_CODE=%d\n",
           inet_ntoa(saddr_other.sin_addr), ntohs(saddr_other.sin_port),
           op_code);

    int err_code = TFTP_SUCCESS;
    if (op_code == TFTP_WRQ) {
      if (TFTP_SUCCESS !=
          (err_code = cmd_put(recieved, recv_len, saddr_other, addrlen))) {
        // graceful_stop(err_code);
      }
    } else if (op_code == TFTP_RRQ) {
      if (TFTP_SUCCESS !=
          (err_code = cmd_get(recieved, recv_len, saddr_other, addrlen))) {
        // graceful_stop(err_code);
      }
    }
  }
  free(recieved);
  graceful_stop(EXIT_SUCCESS);
}

int cmd_put(char *buffer, int length, struct sockaddr_in saddr_other,
            socklen_t addrlen) {
  tftp_wrrq_t wrq;
  char *p = buffer;

  memcpy(&wrq.op_code, p, 2);
  p += 2;

  wrq.file_name = p;
  p += strlen(p) + 1;

  wrq.mode = p;

  printf("WRQ: %s %s\n", wrq.file_name, wrq.mode);

  int fd;
  if (-1 == (fd = openat(root_dirfd, wrq.file_name, O_CREAT | O_EXCL | O_RDWR,
                         0770))) {
    tftp_err_t err_packet;
    err_packet.op_code = TFTP_ERR;
    err_packet.err = TFTP_ERR_FILE_EXISTS;
    err_packet.msg = malloc(128);
    int len =
        sprintf(err_packet.msg, "File '%s' already exists\n", wrq.file_name);
    printf("%s", err_packet.msg);

    uint8_t *err_buf = malloc(len + 1);
    memset(err_buf, 0, len + 1);
    memcpy(err_buf, &err_packet, 4);
    memcpy(err_buf + 4, err_packet.msg, len + 1);
    sendto(sockfd, err_buf, BLOCK_SIZE, 0, (struct sockaddr *)&saddr_other,
           addrlen);

    free(err_buf);
    return fd;
  }

  tftp_ack_t ack_packet;
  ack_packet.op_code = TFTP_ACK;
  ack_packet.block_n = 0;

  sendto(sockfd, &ack_packet, sizeof(ack_packet), 0,
         (struct sockaddr *)&saddr_other, addrlen);

  int read_bytes = 0;
  tftp_data_t data_packet;
  do {
    uint8_t *buffer = malloc(516);
    read_bytes = recvfrom(sockfd, buffer, 516, 0,
                          (struct sockaddr *)&saddr_other, &addrlen);

    // unpacking data packet
    memcpy(&data_packet.op_code, buffer, 2);
    memcpy(&data_packet.block_n, buffer + 2, 2);
    // memcpy(data_packet.data, buffer + 4, BLOCK_SIZE);
    data_packet.data = buffer + 4;

    printf("WRQ: recieved %d bytes from DATA#%d\n-------\n%s\n\n", read_bytes,
           data_packet.block_n, data_packet.data);

    int written;
    if (-1 == (written = write(fd, data_packet.data, read_bytes - 4))) {
      printf("error with writting\n");
    };
    printf("written to %s %d bytes\n", wrq.file_name, written);

    // for (int i = 0; i < read_bytes - 4; ++i) {
    //   printf("%d ", *(data_packet.data + i));
    //   i % 16 == 15 && (printf("\n"));
    // }
    // printf("\n\n");

    ack_packet.block_n = data_packet.block_n;

    sendto(sockfd, &ack_packet, sizeof(ack_packet), 0,
           (struct sockaddr *)&saddr_other, addrlen);
    printf("WRQ: sent ACK#%d\n", ack_packet.block_n);
  } while (read_bytes == 516);

  close(fd);
}

int cmd_get(char *buffer, int length, struct sockaddr_in saddr_other,
            socklen_t addrlen) {
  tftp_wrrq_t rrq;
  char *p = buffer;

  memcpy(&rrq.op_code, p, 2);
  p += 2;

  rrq.file_name = p;
  p += strlen(p) + 1;

  rrq.mode = p;

  printf("RRQ: %s %s\n", rrq.file_name, rrq.mode);

  int fd;
  if (-1 == (fd = openat(root_dirfd, rrq.file_name, O_RDONLY, 0770))) {
    tftp_err_t err_packet;
    err_packet.op_code = TFTP_ERR;
    err_packet.err = TFTP_ERR_FILE_EXISTS;
    err_packet.msg = malloc(128);
    int len =
        sprintf(err_packet.msg, "File '%s' doesn't exists\n", rrq.file_name);
    printf("put: sent [%d] %d %s", err_packet.op_code, err_packet.err,
           err_packet.msg);

    uint8_t *err_buf = malloc(BLOCK_SIZE);
    memcpy(err_buf, &err_packet.op_code, 2);
    memcpy(err_buf + 2, &err_packet.err, 2);
    memcpy(err_buf + 4, err_packet.msg, len + 1);

    sendto(sockfd, err_buf, BLOCK_SIZE, 0, (struct sockaddr *)&saddr_other,
           addrlen);

    free(err_buf);
    return err_packet.err;
  }

  off_t fsize = lseek(fd, 0, SEEK_END);

  tftp_data_t data_packet;
  data_packet.op_code = TFTP_DATA;
  data_packet.block_n = 1;

  tftp_ack_t ack_packet;
  ack_packet.op_code = TFTP_ACK;
  ack_packet.block_n = 0;

  int recv_len;
  for (int sent = 0; sent < fsize; sent += BLOCK_SIZE) {

    // int offset = ack_packet.block_n * BLOCK_SIZE;
    printf("reading from %ld bytes\n", lseek(fd, sent, SEEK_SET));

    data_packet.data = malloc(BLOCK_SIZE);
    int to_sent = read(fd, data_packet.data, BLOCK_SIZE);

    data_packet.op_code = TFTP_DATA;
    uint8_t *packet = malloc(BLOCK_SIZE + 4);
    memcpy(packet, &data_packet, 4);
    // memcpy(packet + 2, &data_packet.block_n, 2);
    memcpy(packet + 4, data_packet.data, to_sent);

    sendto(sockfd, packet, 2 + 2 + to_sent, 0, (struct sockaddr *)&saddr_other,
           addrlen);
    printf("get: sent %d bytes DATA#%d\n", to_sent, data_packet.block_n);

    memset(packet, 0, BLOCK_SIZE + 4);
    recv_len = recvfrom(sockfd, packet, BLOCK_SIZE, 0,
                        (struct sockaddr *)&saddr_other, &addrlen);

    memcpy(&ack_packet, packet, sizeof(ack_packet));
    printf("get: recieved ACK#%d\n", ack_packet.block_n);
    data_packet.block_n = ack_packet.block_n + 1;
  }

  close(fd);
}

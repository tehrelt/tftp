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
  exit(EXIT_SUCCESS);
}

int cmd_put(char *buffer, int length, struct sockaddr_in saddr_other,
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

  // server attributes
  char buffer[2 * BLOCK_SIZE];

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

  // if (0 > listen(sockfd, 3)) {
  //   perror("listen error\n");
  //   exit(EXIT_FAILURE);
  // }

  printf("server started on port %d\n", port);

  int recv_len;

  while (1) {

    printf("Waiting for packets...\n");
    fflush(stdout);

    // if (fork() == 0) {
    // }

    if (-1 ==
        (recv_len = recvfrom(sockfd, buffer, 2 * BLOCK_SIZE, 0,
                             (struct sockaddr *)&saddr_other, &addrlen))) {
      graceful_stop(0);
    }

    // char buffer_ip[16];
    // inet_ntop(AF_INET, &address, buffer_ip, addrlen);

    // int bytes_read = read(current_sockfd, buffer, BLOCK_SIZE);

    uint16_t op_code = 0;
    memcpy(&op_code, buffer, 2);
    printf("Incoming connection from %s:%d OP_CODE=%d\n",
           inet_ntoa(saddr_other.sin_addr), ntohs(saddr_other.sin_port),
           op_code);

    int err_code = TFTP_SUCCESS;
    if (op_code == TFTP_WRQ) {
      if (TFTP_SUCCESS !=
          (err_code = cmd_put(buffer, recv_len, saddr_other, addrlen))) {
        tftp_err_t err_packet;
        err_packet.op_code = TFTP_ERR;
        err_packet.err = err_code;
        err_packet.msg = "";

        if (-1 == (sendto(sockfd, &err_packet, BLOCK_SIZE, 0,
                          (struct sockaddr *)&saddr_other, addrlen))) {
          graceful_stop(EXIT_FAILURE);
        }
      }
    }

    // free(buffer);
    // close(current_sockfd);
  }
  graceful_stop(EXIT_SUCCESS);
}

int cmd_put(char *buffer, int length, struct sockaddr_in saddr_other,
            socklen_t addrlen) {
  tftp_wrrq_t wrq;
  char *p = buffer;

  memcpy(&wrq.op_code, p, 2);
  p += 2;

  wrq.file_name = malloc(BLOCK_SIZE);
  char *fnp = wrq.file_name;
  while (*fnp != '\0') {
    *fnp++ = *p++;
  }
  *fnp != '\0' && (*fnp = *p++); // set 0x0

  wrq.mode = malloc(16);
  char *mnp = wrq.mode;
  while (*p != '\0') {
    *mnp++ = *p++;
  }
  *mnp != '\0' && (*mnp = *p++);

  printf("RRQ: %s %s\n", wrq.file_name, wrq.mode);

  int fd;
  if (0 != (fd == openat(root_dirfd, wrq.file_name, O_CREAT | O_EXCL, 0770))) {
    tftp_err_t err_packet;
    err_packet.op_code = TFTP_ERR;
    err_packet.err = TFTP_ERR_FILE_EXISTS;
    err_packet.msg = malloc(128);

    sprintf(err_packet.msg, "File '%s' already exists", wrq.file_name);
  }

  tftp_ack_t ack_packet;
  ack_packet.op_code = TFTP_ACK;
  ack_packet.block_n = 0;

  sendto(sockfd, &ack_packet, sizeof(ack_packet), 0,
         (struct sockaddr *)&saddr_other, addrlen);

  int read_bytes = 0;
  tftp_data_t data_packet;
  do {
    read_bytes = recvfrom(sockfd, &data_packet, sizeof(tftp_data_t), 0,
                          (struct sockaddr *)&saddr_other, &addrlen);
    printf("WRQ: recieved %d bytes from "
           "DATA#%d\n-------------------------\n%s\n\n-------------------------"
           "-\n",
           read_bytes, data_packet.block_n, data_packet.data);

    write(fd, data_packet.data, read_bytes);

    for (int i = 0; i < read_bytes; ++i) {
      printf("%d ", *(data_packet.data + i));
      i % 16 == 0 && (printf("\n"));
    }

    ack_packet.block_n = data_packet.block_n;

    sendto(sockfd, &ack_packet, sizeof(ack_packet), 0,
           (struct sockaddr *)&saddr_other, addrlen);
    printf("WRQ: sent ACK#%d\n", ack_packet.block_n);
  } while (read_bytes == sizeof(tftp_data_t));

  close(fd);
}

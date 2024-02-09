#include <stdint.h>

#define DEFAULT_PORT 69
// #define BUFFER_SIZE (32 << 20)
#define BLOCK_SIZE 512

#define TFTP_RRQ 1
#define TFTP_WRQ 2
#define TFTP_DATA 3
#define TFTP_ACK 4
#define TFTP_ERR 5

#define TFTP_ERR_UNEXPECTED 0         // 0         Not defined, see error message (if any).
#define TFTP_ERR_FILE_NOT_FOUND 1     // 1         File not found.
#define TFTP_ERR_ACCESS_VIOLATION 2   // 2         Access violation.
#define TFTP_ERR_DISK_FULL 3          // 3         Disk full or allocation exceeded.
#define TFTP_ERR_ILLEGAL 4            // 4         Illegal TFTP operation.
#define TFTP_ERR_UNKNOWN_ID 5         // 5         Unknown transfer ID.
#define TFTP_ERR_FILE_EXISTS 6        // 6         File already exists.

typedef struct tftp_wrrq {
  uint16_t op_code;
  char *file_name;
  char *mode;
} tftp_wrrq_t;

typedef struct tftp_data {
  uint16_t op_code;
  uint16_t block_n;
  char data[BLOCK_SIZE];
} tftp_data_t;

typedef struct tftp_ack {
  uint16_t op_code;
  uint16_t block_n;
} tftp_ack_t;

typedef struct tftp_err {
  uint16_t op_code;
  int err;
  char* msg;
} tftp_err_t;



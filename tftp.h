#include <cstdint>
#include <stdint.h>
#define DEFAULT_PORT 69
#define BUFFER_SIZE (32 << 20)
#define READ_REQUEST 1
#define WRITE_REQUEST 2

typedef struct tftp_wr_message {
  uint8_t op_code;
  char *file_name;
} tftp_wr_message_t;

typedef struct tftp_data_message {
  uint8_t op_code;
  int block_n;
  char *data;
} tftp_data_message_t;

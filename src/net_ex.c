#include "net_ex.h"
#include "prx_proto.h"
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>

ssize_t prx_send(int fd, uint16_t sn, const uint8_t *cipher, uint32_t cl,
                 const uint8_t *sign) {
  uint8_t header[4];
  prx_proto_data_header(sn, header);

  uint8_t len_buf[4];
  uint32_t tot = cl + 16;
  memcpy(len_buf, &tot, 4);

  struct iovec iov[] = {
      [0] = {.iov_base = header, .iov_len = 4},
      [1] = {.iov_base = len_buf, .iov_len = 4},
      [2] = {.iov_base = (uint8_t *)cipher, .iov_len = cl},
      [3] = {.iov_base = (uint8_t *)sign, .iov_len = 16},
  };

  struct msghdr msg;
  memset(&msg, 0x00, sizeof(msg));
  msg.msg_iov = iov;
  msg.msg_iovlen = 4;

  ssize_t sent;
  while ((sent = sendmsg(fd, &msg, 0)) == -1 && errno == EINTR) {
    /*
    见 man 7 signal
    intr发生于signal中断

    在因信号中断后，sendmsg函数总是会触发INTR错误
    无论是否使用过SA_RESTART
    因此才需要重试
    */
  }

  return sent;
}
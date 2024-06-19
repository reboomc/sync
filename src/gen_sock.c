#include "gen_sock.h"
#include "anet.h"
#include "ev.h"
#include "util.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static uint8_t buf[10240];

static void _read_cli(struct aeEventLoop *el, int fd, void *clientData,
                      int mask) {
  UNUSED(mask);

  sock_desc_t *sd = (sock_desc_t *)clientData;

  uint32_t neg1_len;
  int n = anetRead(fd, buf, sizeof(buf), &neg1_len);

  if (0 == n) {
    close(fd);
    aeDeleteFileEvent(el, fd, AE_READABLE);
    if (sd->on_close)
      sd->on_close(sd, fd);
    return;
  }

  if (-1 == n && (EAGAIN == errno || EWOULDBLOCK == errno)) {
    n = neg1_len;
  }
  if (n > 0) {
    sd->recv_notify(sd, fd, buf, n);
  }
}

static void _accept(struct aeEventLoop *el, int fd, void *clientData,
                    int mask) {
  UNUSED(mask);

  sock_desc_t *sd = (sock_desc_t *)clientData;

  int cli_fd = sd->accept(sd->arg, fd);
  if (cli_fd <= 0)
    return;

  anetNonBlock(NULL, cli_fd);
  aeCreateFileEvent(el, cli_fd, AE_READABLE, _read_cli, sd);
}

int gen_svr_create(sock_desc_t *sd, aeEventLoop *el, void *arg) {
  sd->arg = arg;
  int fd = sd->create_poll(sd->arg);

  if (fd > 0) {
    aeCreateFileEvent(el, fd, AE_READABLE, _accept, sd);
  } else {
    fprintf(stderr, "maybe port used\n");
  }
  return fd;
}

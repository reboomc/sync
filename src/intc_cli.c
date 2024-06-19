#include "intc_cli.h"
#include "anet.h"
#include "intc_con.h"
#include "log.h"
#include "unix_sock.h"
#include "util.h"
#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define USE_LL_CACHE_NO

struct _intcli {
  int fd;
};

static intc_cli_t *_start(const char *us_path) {
  int fd = anetUnixConnect(NULL, us_path);
  if (fd <= 0) {
    robot_log(LOG_W, "us conn fail");
    return NULL;
  }

  intc_cli_t *cli = malloc(sizeof(*cli));
  cli->fd = fd;

  return cli;
}

intc_cli_t *intc_fetch(const char *us_path) {
  if (!us_path)
    return NULL;

  return
#ifdef USE_LL_CACHE
      // 使用链表作为缓存
      intc_fetch_ll(us_path, _start);
#else
      intc_fetch_dict(us_path, _start);
#endif
}

static void _close(intc_cli_t *cli) {
  if (!cli)
    return;

  close(cli->fd);
  free(cli);
}

void intc_dispose(void) {
#ifdef USE_LL_CACHE
  intc_dispose_ll(_close);
#else
  intc_dispose_dict(_close);
#endif
}

void intc_send(const intc_cli_t *cli, const uint8_t *buf, uint32_t len) {
  if (!cli || !buf || !len) {
    return;
  }
  int n = anetWrite(cli->fd, buf, len);
  UNUSED(n);
}

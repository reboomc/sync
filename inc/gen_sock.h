#pragma once

#include "ev.h"
#include <stdint.h>

struct _sock_desc;

typedef struct _sock_desc {
  void *arg;
  void (*recv_notify)(const struct _sock_desc *cli, int fd, const uint8_t *buf,
                      int n);
  int (*accept)(void *arg, int fd);
  void (*on_close)(const struct _sock_desc *cli, int fd);
  int (*create_poll)(void *arg);
} sock_desc_t;

int gen_svr_create(sock_desc_t *sd, aeEventLoop *el, void *arg);
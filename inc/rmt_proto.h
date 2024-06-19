#pragma once

#include "gen_sock.h"
#include <stdint.h>

void rmt_proto_notify(void *arg, int fd, const uint8_t *buf, uint32_t len);
void rmt_proto_clean();
// 连接端口后的处理
void rmt_proto_interrupt(const struct _sock_desc *sd, int fd);
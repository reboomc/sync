#pragma once

#include <stdint.h>
#include <sys/types.h>

/*
向远端发送prx报文
@return 0: 成功
- 1: 需重试
 */
ssize_t prx_send(int fd, uint16_t sn, const uint8_t *cipher, uint32_t cl,
                 const uint8_t *sign);
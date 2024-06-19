#pragma once

#include <stdint.h>

struct fh_product {
  uint8_t id;
  uint16_t sn;
  uint32_t len;
  const uint8_t *buf;
};

void *prx_resp_parse(const uint8_t *buf, uint32_t len, int *cost);
void *rmt_parse(const uint8_t *buf, uint32_t len, int *cost);
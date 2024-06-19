#include "fh_proto.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

// 数据块长度指征
#define ID_DLEN 4

static struct fh_product *_parse(const uint8_t *buf, uint32_t len,
                                 int32_t *cost) {
  *cost = 0;
  if (len < 4)
    return NULL;

  if ('*' != *buf) {
    robot_log(LOG_W, "preamble inv: %d", *buf);
    *cost = -1;
    return NULL;
  }

  uint8_t id = *(buf + 1);
  if (id > 1) {
    robot_log(LOG_W, "preamble#2 inv: %d", id);
    *cost = -2;
    return NULL;
  }

  uint16_t sn;
  memcpy(&sn, buf + 2, 2);

  struct fh_product *prod = (struct fh_product *)malloc(sizeof(*prod));

  prod->id = id;
  prod->sn = sn;
  prod->buf = NULL;
  prod->len = 0;

  // 无论是否后续处理，cost值需更新
  *cost = 4;
  return prod;
}

void *prx_resp_parse(const uint8_t *buf, uint32_t len, int32_t *cost) {
  return _parse(buf, len, cost);
}

static uint8_t _rmt_data(const uint8_t *buf, uint32_t len, int32_t *cost,
                         struct fh_product *prod) {
  *cost = 0;
  if (len < ID_DLEN)
    return 0;

  uint32_t pl;
  memcpy(&pl, buf, ID_DLEN);

  uint32_t tot = ID_DLEN + pl;
  if (tot > len) {
    return 0;
  }

  *cost = 4 /*在此之前的4字节头部*/ + tot;
  prod->buf = buf + ID_DLEN;
  prod->len = pl;
  return 1;
}

void *rmt_parse(const uint8_t *buf, uint32_t len, int32_t *cost) {
  struct fh_product *p = _parse(buf, len, cost);
  if (!p)
    return NULL;

  if (0 == p->id) {
    // 心跳协议
    return p;
  }

  if (len < ID_DLEN + (uint32_t)*cost) {
    // 既然后续数据块未处理，不应占用字节
    *cost = 0;
    free(p);
    return NULL;
  }

  if (!_rmt_data(buf + *cost, len - *cost, cost, p)) {
    free(p);
    return NULL;
  }
  return p;
}
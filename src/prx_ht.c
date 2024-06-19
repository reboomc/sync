#include "prx_ht.h"
#include "bitmap.h"
#include "gen_buf.h"
#include "log.h"
#include "stddef.h"
#include "vector.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 用于维护intc进入的fd和buf关系
typedef struct _prx_ht {
  linked_list_t *vector;
  gen_buf_t *gb;
} prx_node_t;

struct _prx_bucket {
  // 记录哈希表索引是否被初始化
  bmap_t *desc;
  uint32_t mod;
  prx_node_t *slots;
};

static uint8_t _init = 0;
static struct _prx_bucket _bucket = {0x00, 0x00, 0x00};

static void _conf() {
  uint32_t ncpu = sysconf(_SC_NPROCESSORS_ONLN);

  uint32_t tot = ncpu * 11;
  // 每个槽均未被填充
  _bucket.desc = bmap_new(tot);
  _bucket.mod = tot + 1;
  _bucket.slots = malloc(sizeof(prx_node_t) * tot);
}

static int32_t _sel_item(int key) {
  // us服务端的fd从8开始
  if (key < 8)
    return -1;
  return (key - 8) % _bucket.mod;
}

static void *_extract(const uint8_t *buf, uint32_t len, int32_t *cost) {
#define BC 4

  *cost = 0;
  if (len < BC)
    return NULL;

  // 报文的数据长度
  uint32_t tot;
  memcpy(&tot, buf, BC);

  if (len < BC + tot)
    return NULL;

  *cost = BC + tot;

  struct prxi *p = malloc(sizeof(*p));
  p->buf = buf + BC;
  p->tot = tot;
  return p;
}

void prx_ht_reset(int key) {
  if (!_init)
    return;

  int32_t pos = _sel_item(key);
  if (-1 == pos)
    return;

  if (!bmap_get(_bucket.desc, (uint32_t)pos))
    return;

  gen_buf_reset((_bucket.slots + pos)->gb);
}

linked_list_t *prx_ht_append(int key, const uint8_t *buf, uint32_t len) {
  if (!_init) {
    _init = 1;
    _conf();
  }

  if (!buf || !len)
    return NULL;

  int32_t pos = _sel_item(key);
  if (-1 == pos)
    return NULL;

  uint8_t untouch = !bmap_get(_bucket.desc, (uint32_t)pos);
  if (untouch) {
    // 该节点还未填充
    bmap_set(_bucket.desc, (uint32_t)pos, 1);
  }

  prx_node_t *node = _bucket.slots + pos;
  if (untouch) {
    node->gb = gen_buf_new(20480, _extract);
    node->vector = NULL;
    new_linklist(&node->vector);
  }

  gen_buf_in(node->gb, buf, len, node->vector);
  return node->vector;
}

void prx_ht_clean() {
  if (!_init)
    return;

  for (uint32_t i = 0; i < _bucket.mod - 1; i++) {
    if (!bmap_get(_bucket.desc, i))
      continue;

    prx_node_t *node = _bucket.slots + i;
    // 节点对象直接free即可
    free_items(node->vector, NULL, 0, NULL);
    gen_buf_free(node->gb);
  }

  bmap_free(_bucket.desc);
  free(_bucket.slots);
}

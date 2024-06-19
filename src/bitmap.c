#include "bitmap.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct _bmap {
  uint8_t *bits;
  uint32_t len;
};

bmap_t *bmap_new(uint16_t bytes) {
  bmap_t *b = (bmap_t *)malloc(sizeof(*b));
  b->len = bytes * 8;
  b->bits = (uint8_t *)malloc(bytes);
  memset(b->bits, 0x00, bytes);

  return b;
}

static uint32_t _offset2idx(uint32_t o) { return o >> 3; }

void bmap_set(bmap_t *b, uint32_t pos, uint8_t on) {
  if (!b || pos >= b->len)
    return;

  uint32_t offset = _offset2idx(pos);
  uint8_t old_byte = *(b->bits + offset);
  uint8_t bit = 7 - (pos & 0x7);

  uint8_t val = old_byte;
  // 先置0
  val &= ~(1 << bit);
  // 再按照给定值设置
  val |= ((on && 0x1) << bit);

  *(b->bits + offset) = val;
}

uint8_t bmap_get(const bmap_t *b, uint32_t pos) {
  if (!b || pos >= b->len)
    return 0;

  uint8_t byte = *(b->bits + _offset2idx(pos));
  uint8_t bit = 7 - (pos & 0x7);
  return byte & (1 << bit) ? 1 : 0;
}

void bmap_free(bmap_t *b) {
  if (!b)
    return;

  free(b->bits);
  free(b);
}
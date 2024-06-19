#include "gen_buf.h"
#include "log.h"
#include "util.h"
#include "vector.h"
#include <bits/pthreadtypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _gen_buf {
  uint32_t total;
  uint32_t offset;
  // 该段业务数据开始的位置
  uint32_t block_start;
  uint8_t *pool;
  // 兼容两种协议格式
  data_consume dc_func;
};

gen_buf_t *gen_buf_new(uint32_t init_len, data_consume dc) {
  gen_buf_t *o = (gen_buf_t *)malloc(sizeof(*o));

  o->total = init_len;
  o->pool = malloc(o->total);
  o->offset = 0;
  o->block_start = 0;
  o->dc_func = dc;

  return o;
}

/*
@return <0: error
0: not resolved yet
1: produced
 */
static inline void _try_block(const uint8_t *buf, uint32_t len, int32_t *cost,
                              linked_list_t *list, data_consume dc) {
  if (!dc) {
    *cost = -1;
    return;
  }

  void *prod = dc(buf, len, cost);
  if (prod) {
    append_item(list, prod, NULL);
  }
}

char gen_buf_in(gen_buf_t *rb, const uint8_t *in, uint32_t len_in,
                linked_list_t *list) {
  if (!rb)
    return 1;
  /*
  +-----------+---+-------+
  +  <free>  bs   o      tot
   */

  if (rb->offset + len_in >= rb->total) {
    uint32_t freespace = rb->block_start;
    // 尚未决议的长度
    uint32_t nr_len = rb->offset - rb->block_start;

    if (freespace >= nr_len + len_in) {
      memmove(rb->pool, rb->pool + rb->block_start, nr_len);
      rb->block_start = 0;
      rb->offset = nr_len;
    } else {
      uint32_t new_tot = (uint32_t)(rb->total * 1.6);
      new_tot = MAX(new_tot, rb->total + len_in);

      rb->pool = realloc(rb->pool, new_tot);
      rb->total = new_tot;
    }
  }

  memcpy(rb->pool + rb->offset, in, len_in);
  rb->offset += len_in;

  int32_t cost = 0;
  while (1) {
    _try_block(rb->pool + rb->block_start, rb->offset - rb->block_start, &cost,
               list, rb->dc_func);

    if (cost <= 0) {
      if (cost < 0) {
        rb->offset = rb->block_start;
        robot_log(LOG_N, "try fail, offset back to %d", rb->block_start);
        return -1;
      }
      break;
    }

    rb->block_start += cost;
  }
  return 1;
}

void gen_buf_free(gen_buf_t *rb) {
  if (!rb)
    return;

  free(rb->pool);
  free(rb);
}

void gen_buf_reset(gen_buf_t *rb) {
  if (!rb)
    return;

  rb->offset = rb->block_start;
  robot_log(LOG_D, "on rmt close, reset offset %d", rb->block_start);
}
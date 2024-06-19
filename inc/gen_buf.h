#pragma once

#include "vector.h"
#include <bits/pthreadtypes.h>
#include <stdint.h>

typedef struct _gen_buf gen_buf_t;

typedef void *(*data_consume)(const uint8_t *buf, uint32_t len, int32_t *cost);

gen_buf_t *gen_buf_new(uint32_t init_len, data_consume dc_func);
/*
接收进入的报文并解析
@return <0: error
- 0: 等待
- 1: 成功
 */
char gen_buf_in(gen_buf_t *rb, const uint8_t *buf, uint32_t len,
                linked_list_t *list);
void gen_buf_free(gen_buf_t *rb);
void gen_buf_reset(gen_buf_t *rb);
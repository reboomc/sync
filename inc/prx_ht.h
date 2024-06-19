#pragma once

#include "vector.h"
#include <stdint.h>

struct prxi {
  const uint8_t *buf;
  uint32_t tot;
};

/*
追加已收到的报文
@param key: 通常用fd填充
@param tot: 所有已追加的长度
@return 当前报文是否完整
 */
linked_list_t *prx_ht_append(int key, const uint8_t *buf, uint32_t len);
void prx_ht_reset(int key);
void prx_ht_clean();
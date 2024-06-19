#pragma once

#include "redis_cli.h"
#include <stdint.h>

typedef struct _rmt_agent rmt_agent_t;

typedef struct _rmt_ctx {
  char *mongo_host;
  uint16_t listen_port;
  // redis目标数量
  uint8_t red_cnt;
  // 白名单数量
  uint8_t wl_len;
  char **ip_list;
  // redis配置, 预估数量
  struct _redis_cfg *red_list[4];
} rmt_ctx_t;

rmt_agent_t *rmt_agent_new(const rmt_ctx_t *ctx);
/*
追加业务报文
@param payload: 完整的报文
@param len: 长度
 */
void rmt_agent_append(rmt_agent_t *, uint16_t sn, const uint8_t *payload,
                      uint32_t len);
const rmt_ctx_t *rmt_agent_ctx(const rmt_agent_t *a);
void rmt_agent_free(rmt_agent_t **pa);
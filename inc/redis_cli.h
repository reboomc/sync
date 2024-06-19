#pragma once

#include "ev.h"
#include <stdint.h>

struct _redis_cfg {
  char host[32];
  uint16_t port;
  uint16_t db;
  char pwd[16];
  // 用于匹配map字段
  char section[16];
  uint16_t alias_port;
  uint16_t alias_db;
};

struct _redis_many_desc {
  uint16_t offset;
  uint16_t len;
};

typedef struct _redis_cli redis_cli_t;

redis_cli_t *redis_prefetch(aeEventLoop *el, uint8_t cnt,
                            const struct _redis_cfg **cfg_list);
// 单条指令
uint8_t redis_one(const redis_cli_t *cli, const uint8_t *cmd, uint32_t len);
/*
执行多条指令
@param cnt: 数量
@param list: 指令集
 */
uint8_t redis_many(const redis_cli_t *cli, const uint8_t *full_expr,
                   uint8_t cnt, const struct _redis_many_desc *md_list);
/*
查找合适的redis连接对象
@param cnt: 配置数量
 */
const redis_cli_t *redis_find(uint16_t port, uint8_t db, uint8_t cnt,
                              const redis_cli_t *list);
void redis_free(aeEventLoop *el, uint8_t cnt, redis_cli_t *cli);
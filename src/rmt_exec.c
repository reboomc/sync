#include "rmt_exec.h"
#include "log.h"
#include <stdint.h>
#include <stdlib.h>

#define ENABLE_MONGO 1

#if ENABLE_MONGO
#include "cJSON.h"
#include <bson/bson.h>
#include <mongoc/mongoc.h>
#include <stdio.h>
#endif
#include "redis_cli.h"
#include <string.h>

#if ENABLE_MONGO

static uint8_t _mongo_init = 0;
static mongoc_client_t *_mg_cli = NULL;
typedef void (*mg_func)(mongoc_collection_t *col, const cJSON *vpl);

#define MG_JSON_BSON_BEGIN()                                                   \
  char *data = cJSON_PrintUnformatted(vpl);                                    \
  bson_error_t error;                                                          \
  bson_t *doc = bson_new_from_json((const uint8_t *)data, -1, &error);         \
  if (doc) {

#define MG_JSON_BSON_END()                                                     \
  bson_destroy(doc);                                                           \
  }                                                                            \
  free(data);

/*
插入记录
@param fo: root's field of object
 */
static void _mg_run(const cJSON *root, const cJSON *fo, mg_func mf) {
  if (!mf)
    return;
  mongoc_collection_t *col = mongoc_client_get_collection(
      _mg_cli, cJSON_GetObjectItem(root, "db")->valuestring,
      cJSON_GetObjectItem(root, "tab")->valuestring);

  mf(col, cJSON_GetObjectItem(fo, "v"));
  mongoc_collection_destroy(col);
}

// @param vpl: v payload
static void _mg_insert(mongoc_collection_t *col, const cJSON *vpl) {
  MG_JSON_BSON_BEGIN()
  mongoc_collection_insert_one(col, doc, NULL, NULL, &error);
  MG_JSON_BSON_END()
}

static void _mg_insert_many(mongoc_collection_t *col, const cJSON *vpl) {
  if (!cJSON_IsArray(vpl))
    return;

  int tot = cJSON_GetArraySize(vpl);
  for (int i = 0; i < tot; i++) {
    _mg_insert(col, cJSON_GetArrayItem(vpl, i));
  }
}

static void _mg_delete_many(mongoc_collection_t *col, const cJSON *vpl) {
  MG_JSON_BSON_BEGIN()
  mongoc_collection_delete_many(col, doc, NULL, NULL, &error);
  MG_JSON_BSON_END()
}

static void _mongo(const rmt_ctx_t *ctx, const uint8_t *buf, uint32_t len) {
#define MATCH(cm) 0 == strcmp(method, cm)

  if (!_mongo_init) {
    _mongo_init = 1;
    mongoc_init();
    _mg_cli = mongoc_client_new(ctx->mongo_host);
  }

  cJSON *j = cJSON_ParseWithLength((const char *)buf, len);
  cJSON *j_wrap = cJSON_GetObjectItem(j, "o");

  const char *method = cJSON_GetObjectItem(j_wrap, "mtd")->valuestring;
  mg_func mf = NULL;

  if (MATCH("insert")) {
    mf = _mg_insert;
  } else if (MATCH("insertm")) {
    mf = _mg_insert_many;
  } else if (MATCH("del")) {
    mf = _mg_delete_many;
  }
  _mg_run(j, j_wrap, mf);

  cJSON_Delete(j);
}

#endif

void rmt_agent_close() {
#if ENABLE_MONGO

  if (!_mongo_init)
    return;
  mongoc_client_destroy(_mg_cli);
  mongoc_cleanup();
#endif
}

/*
解析redis打包格式
@multi: 是否多条指令
@return 指令条数
 */
static uint8_t _parse_red(const uint8_t *buf, uint16_t *port, uint8_t *db,
                          uint32_t *out_start, uint8_t *multi) {
  const char *s = (const char *)buf;
  char *comma = strstr(s, ",");

  *port = (uint16_t)atoi(s);
  // 跳过逗号取数据库索引
  *db = (uint8_t)atoi(comma + 1);
  // 跳过db取数量
  uint8_t skip = *db >= 10 ? 2 : 1;
  uint8_t cnt = *(comma + 1 /*逗号自身*/ + skip);

  // 对于单条指令，返回偏移量即可
  *out_start = (comma + 1 + skip + 1 /*数量*/) - s;
  *multi = cnt >> 7;
  return cnt & 0x7f;
}

/*
解析多条
@param cnt: 指令数量
 */
static void _many_red(const uint8_t *buf, uint8_t cnt,
                      struct _redis_many_desc *desc_list) {
  uint32_t start = 0;
  for (uint8_t i = 0; i < cnt; i++) {
    uint16_t cmd_len;
    memcpy(&cmd_len, buf + start, 2);

    const uint8_t *expr = buf + start + 2 /*长度指征*/;

    struct _redis_many_desc *ed = desc_list + i;
    ed->offset = expr - buf;
    ed->len = cmd_len;

    start += cmd_len + 2;
  }
}

void rmt_agent_exec(const redis_cli_t *rdc_list, const rmt_ctx_t *ctx,
                    const uint8_t *buf, uint32_t len) {
  if ('{' == *buf) {
#if ENABLE_MONGO
    _mongo(ctx, buf, len);
#endif
    return;
  }

  uint16_t port;
  uint8_t db;
  uint32_t start;
  uint8_t multi;

  uint8_t cnt = _parse_red(buf, &port, &db, &start, &multi);
  const redis_cli_t *cli = redis_find(port, db, ctx->red_cnt, rdc_list);
  if (!cli) {
    robot_log(LOG_W, "no red cli: %d, %d", port, db);
    return;
  }

  if (!multi) {
    redis_one(cli, buf + start, len - start);
    return;
  }

  struct _redis_many_desc rd_md[cnt];
  _many_red(buf + start, cnt, rd_md);
  redis_many(cli, buf + start, cnt, rd_md);
}

#include "rmt_agent.h"
#include "cipher.h"
#include "ev.h"
#include "log.h"
#include "md5.h"
#include "redis_cli.h"
#include "rmt_exec.h"
#include "util.h"
#include "vector.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static const uint8_t aes_secret[] = AES_SECRET;

struct _rmt_agent {
  pthread_t pid;
  int evt_in;
  int evt_out;

  linked_list_t *data_cache;
  aeEventLoop *el;
  const rmt_ctx_t *ctx;
  redis_cli_t *rd_cli_list;
  pthread_mutex_t mutex;
};

struct _db_obj {
  uint8_t *cipher;
  uint32_t len;
  uint16_t sn;
};

static void _free_db(void *db) {
  struct _db_obj *o = (struct _db_obj *)db;
  if (!o)
    return;

  free(o->cipher);
  free(o);
}

static void _data_new(struct aeEventLoop *el, int _, void *arg, int mask) {
  UNUSED(el);
  UNUSED(_);
  UNUSED(mask);

  rmt_agent_t *a = (rmt_agent_t *)arg;

  uint8_t pipe_temp[1];
  recv(a->evt_out, pipe_temp, 1, 0);

  struct _db_obj *db;
  if (!pop_item_count(a->data_cache, 1, (void **)&db, &a->mutex, 1)) {
    // 断开时发生
    robot_log(LOG_D, "no data");
    return;
  }

  if (db->len < 32) {
    _free_db(db);
    robot_log(LOG_D, "len %d", db->len);
    return;
  }

  size_t len_plain;
  uint8_t *plain = decrypt(db->cipher, db->len - 16, aes_secret, &len_plain);

  if (!plain) {
    _free_db(db);
    robot_log(LOG_D, "decrypt fail");
    return;
  }

  uint8_t expect_sign[16];
  md5(plain, len_plain, expect_sign);

  if (0 != memcmp(expect_sign, db->cipher + db->len - 16, 16)) {
    free(plain);
    _free_db(db);
    robot_log(LOG_D, "sign inv");
    return;
  }

  free(db->cipher);
  db->cipher = plain;
  db->len = len_plain;

  rmt_agent_exec(a->rd_cli_list, a->ctx, plain, len_plain);
  _free_db(db);
}

static void *_agent_routine(void *arg) {
  rmt_agent_t *a = (rmt_agent_t *)arg;

  aeMain(a->el);
  aeDeleteFileEvent(a->el, a->evt_out, AE_READABLE);
  aeDeleteEventLoop(a->el);

  return NULL;
}

rmt_agent_t *rmt_agent_new(const rmt_ctx_t *ctx) {
  int pair[2];
  if (0 != socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) {
    return NULL;
  }

  rmt_agent_t *a = malloc(sizeof(*a));
  a->pid = 0;
  a->evt_in = pair[0];
  a->evt_out = pair[1];

  a->ctx = ctx;
  a->data_cache = NULL;
  new_linklist(&a->data_cache);

  a->el = aeCreateEventLoop(15);
  aeCreateFileEvent(a->el, a->evt_out, AE_READABLE, _data_new, a);

  a->rd_cli_list = redis_prefetch(a->el, ctx->red_cnt,
                                  (const struct _redis_cfg **)ctx->red_list);
  pthread_mutex_init(&a->mutex, NULL);
  pthread_create(&a->pid, NULL, _agent_routine, a);
  return a;
}

static void _notify_pipe(const rmt_agent_t *agent) {
  uint8_t signal[] = {0x00};
  ssize_t n = write(agent->evt_in, signal, 1);
  UNUSED(n);
}

const rmt_ctx_t *rmt_agent_ctx(const rmt_agent_t *a) { return a->ctx; }

void rmt_agent_append(rmt_agent_t *agent, uint16_t sn, const uint8_t *payload,
                      uint32_t len) {
  struct _db_obj *o = (struct _db_obj *)malloc(sizeof(*o));

  o->cipher = malloc(len);
  memcpy(o->cipher, payload, len);
  o->len = len;
  o->sn = sn;

  append_item(agent->data_cache, o, &agent->mutex);
  _notify_pipe(agent);
}

void rmt_agent_free(rmt_agent_t **pa) {
  if (!pa)
    return;

  rmt_agent_t *agent = *pa;
  if (!agent)
    return;

  redis_free(agent->el, agent->ctx->red_cnt, agent->rd_cli_list);
  aeStop(agent->el);
  if (0 != agent->pid) {
    _notify_pipe(agent);
    pthread_join(agent->pid, NULL);
  }

  free_items(agent->data_cache, _free_db, 0, &agent->mutex);
  free(agent);
}
#include "redis_cli.h"
#include "anet.h"
#include "ev.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MULTI "*1\r\n$5\r\nMULTI\r\n"
#define EXEC "*1\r\n$4\r\nEXEC\r\n"

struct _redis_cli {
  uint16_t port;
  uint8_t db;
  uint8_t match_db;
  uint16_t match_port;
  int fd;
};

static void _red_recv(struct aeEventLoop *el, int fd, void *arg, int mask) {
  UNUSED(mask);

  uint8_t buf[64];

  int n = anetRead(fd, buf, sizeof(buf), NULL);
  if (0 == n) {
    close(fd);
    aeDeleteFileEvent(el, fd, AE_READABLE);

    redis_cli_t *cli = (redis_cli_t *)arg;
    cli->fd = -1;
  }
}

static void _auth(char *buf, uint8_t len, const char *pwd) {
  snprintf(buf, len, "*2\r\n$4\r\nAUTH\r\n$%d\r\n%s\r\n", (uint8_t)strlen(pwd),
           pwd);
}

static void _select(char *buf, uint8_t len, uint16_t db) {
  snprintf(buf, len, "*2\r\n$6\r\nSELECT\r\n$%d\r\n%d\r\n", db >= 10 ? 2 : 1,
           db);
}

redis_cli_t *redis_prefetch(aeEventLoop *el, uint8_t cnt,
                            const struct _redis_cfg **cfg_list) {
  if (!cfg_list || !cnt)
    return NULL;

  redis_cli_t *list = malloc(sizeof(redis_cli_t) * cnt);

  char buf[128];

  for (uint8_t i = 0; i < cnt; i++) {
    const struct _redis_cfg *cfg = *(cfg_list + i);

    redis_cli_t *each = list + i;
    if (cfg->alias_port) {
      each->port = cfg->alias_port;
      each->db = cfg->alias_db;
    } else {
      each->port = cfg->port;
      each->db = cfg->db;
    }
    each->fd = anetTcpConnect(NULL, cfg->host, each->port);

    // 仅用于匹配
    each->match_port = cfg->port;
    each->match_db = cfg->db;

    if (each->fd > 0) {
      anetNonBlock(NULL, each->fd);
      anetKeepAlive(NULL, each->fd, 12);

      aeCreateFileEvent(el, each->fd, AE_READABLE, _red_recv, each);

      if (strlen(cfg->pwd)) {
        _auth(buf, sizeof(buf), cfg->pwd);
        anetWrite(each->fd, (const uint8_t *)buf, strlen(buf));
      }
      if (0 != each->db) {
        _select(buf, sizeof(buf), each->db);
        anetWrite(each->fd, (const uint8_t *)buf, strlen(buf));
      }
    }
  }

  return list;
}

uint8_t redis_one(const redis_cli_t *cli, const uint8_t *cmd, uint32_t len) {
  if (cli->fd < 0 || !cmd)
    return 0;

  anetWrite(cli->fd, cmd, len);
  return 1;
}

uint8_t redis_many(const redis_cli_t *cli, const uint8_t *full_expr,
                   uint8_t cnt, const struct _redis_many_desc *md_list) {
  uint32_t lm = strlen(MULTI);
  uint32_t le = strlen(EXEC);

  if (cli->fd < 0 || !cnt || !full_expr || !md_list)
    return 0;

  const char *multi = MULTI;
  const char *exec = EXEC;

  anetWrite(cli->fd, (const uint8_t *)multi, lm);

  for (uint8_t i = 0; i < cnt; i++) {
    const struct _redis_many_desc *ed = md_list + i;
    anetWrite(cli->fd, full_expr + ed->offset, ed->len);
  }

  anetWrite(cli->fd, (const uint8_t *)exec, le);
  return 1;
}

const redis_cli_t *redis_find(uint16_t port, uint8_t db, uint8_t cnt,
                              const redis_cli_t *list) {
  if (!list)
    return NULL;

  for (uint8_t i = 0; i < cnt; i++) {
    const redis_cli_t *each = list + i;
    if (each->match_port == port && each->match_db == db)
      return each;
  }

  return NULL;
}

void redis_free(aeEventLoop *el, uint8_t cnt, redis_cli_t *list) {
  if (!list)
    return;

  for (uint8_t i = 0; i < cnt; i++) {
    redis_cli_t *each = list + i;
    if (each->fd > 0) {
      aeDeleteFileEvent(el, each->fd, AE_READABLE);
      close(each->fd);
    }
  }
  free(list);
}
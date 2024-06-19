#include "ev.h"
#include "gen_sock.h"
#include "ini.h"
#include "redis_cli.h"
#include "rmt_agent.h"
#include "rmt_exec.h"
#include "rmt_proto.h"
#include "rmt_sock.h"
#include "util.h"
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static aeEventLoop *el = NULL;

static void _on_recv(const sock_desc_t *sd, int fd, const uint8_t *buf, int n) {
  rmt_proto_notify(sd->arg, fd, buf, n);
}

static void _loop(rmt_agent_t *a) {
  el = aeCreateEventLoop(64);

  sock_desc_t sd;
  sd.recv_notify = _on_recv;
  sd.accept = rmt_svr_accept;
  sd.create_poll = rmt_svr_create;
  sd.on_close = rmt_proto_interrupt;

  int tcp_fd = gen_svr_create(&sd, el, a);
  aeMain(el);
  aeDeleteEventLoop(el);
  if (tcp_fd > 0) {
    close(tcp_fd);
  }
}

static void sig_int(int sig) {
  UNUSED(sig);
  aeStop(el);
}

static void _free_parts(char **parts, uint8_t cnt) {
  if (!parts)
    return;
  for (uint8_t i = 0; i < cnt; i++) {
    free(*(parts + i));
  }
  free(parts);
}

static struct _redis_cfg *_match_cfg(const char *section,
                                     const rmt_ctx_t *ctx) {
  for (uint8_t i = 0; i < ctx->red_cnt; i++) {
    struct _redis_cfg *cfg = ctx->red_list[i];
    if (0 == strcmp(cfg->section, section))
      return cfg;
  }

  return NULL;
}

static int _cfg_handler(void *user, const char *section, const char *name,
                        const char *value) {
  rmt_ctx_t *ctx = (rmt_ctx_t *)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
#define FREE_REMATCH()                                                         \
  for (uint8_t i = 0; i < capture_count; i++) {                                \
    free(*(results + i));                                                      \
  }                                                                            \
  free(results);

  if (MATCH("listen", "port")) {
    ctx->listen_port = atoi(value);
  } else if (MATCH("wl", "ip")) {
    ctx->ip_list = str_split(value, ",", &ctx->wl_len);
  } else if (MATCH("mongo", "host")) {
    ctx->mongo_host = strdup(value);
  } else if (strstr(section, "redis") && !strcmp(name, "host")) {
    uint8_t capture_count = 4;
    char **results = re_search("^redis://([^:]+):(\\d+)/(\\d+)\\?pwd=(.*)$",
                               value, capture_count);
    if (!results)
      return 0;

    struct _redis_cfg *cfg = malloc(sizeof(*cfg));
    ctx->red_list[ctx->red_cnt++] = cfg;
    cfg->alias_db = 0;
    cfg->alias_port = 0;
    snprintf(cfg->section, sizeof(cfg->section), "%s", section);

    snprintf(cfg->host, sizeof(cfg->host), "%s", *results);
    cfg->port = (uint16_t)atoi(*(results + 1));
    cfg->db = (uint16_t)atoi(*(results + 2));
    snprintf(cfg->pwd, sizeof(cfg->pwd), "%s", *(results + 3));

    FREE_REMATCH()
  } else if (strstr(section, "redis") && !strcmp(name, "map")) {
    struct _redis_cfg *cfg = _match_cfg(section, ctx);
    if (!cfg)
      return 0;

    uint8_t capture_count = 2;
    char **results = re_search("^(\\d{4,5})/(\\d{1,2})$", value, capture_count);
    if (!results)
      return 0;

    cfg->alias_port = (uint16_t)atoi(*results);
    cfg->alias_db = (uint16_t)atoi(*(results + 1));
    FREE_REMATCH()
  }

  return 1;
}

static uint8_t _parse_cfg(const char *fn, rmt_ctx_t *ctx) {
  if (0 != access(fn, F_OK))
    return 0;

  if (ini_parse(fn, _cfg_handler, ctx) < 0) {
    return 0;
  }
  if (0 == ctx->listen_port || 0 == ctx->wl_len)
    return 0;
  return 1;
}

static void _free_ctx(rmt_ctx_t *ctx) {
  _free_parts(ctx->ip_list, ctx->wl_len);
  free(ctx->mongo_host);
  for (uint8_t i = 0; i < ctx->red_cnt; i++) {
    free(ctx->red_list[i]);
  }
}

// 远端服务
int main(int argc, const char **argv) {
  char *cwd = resolve_cwd(*argv);
  if (cwd) {
    int _ = chdir(cwd);
    UNUSED(_);
    free(cwd);
  }

  if (2 != argc) {
    fprintf(stderr, "usage: %s <cfg>\n", *argv);
    return 1;
  }

  rmt_ctx_t ctx;
  memset(&ctx, 0x00, sizeof(ctx));

  if (!_parse_cfg(*(argv + 1), &ctx)) {
    fprintf(stderr, "parse cfg fail\n");
    return 1;
  }
  signal(SIGINT, sig_int);
  signal(SIGINT, SIG_IGN);

  rmt_agent_t *a = rmt_agent_new(&ctx);
  _loop(a);
  rmt_agent_free(&a);
  rmt_proto_clean();
  _free_ctx(&ctx);
  rmt_agent_close();

  return 0;
}
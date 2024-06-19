#include "ev.h"
#include "gen_sock.h"
#include "ini.h"
#include "prx_cli.h"
#include "prx_ht.h"
#include "spt.h"
#include "unix_sock.h"
#include "util.h"
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static aeEventLoop *el = NULL;

struct prx_cfg {
  char *us_path;
  char *host;
  uint16_t port;
};

static void _on_recv(const sock_desc_t *sd, int fd, const uint8_t *buf, int n) {
  prx_cli_notify((prx_cli_t *)sd->arg, fd, buf, n);
}

static void _on_close(const sock_desc_t *sd, int fd) {
  prx_cli_t *cli = (prx_cli_t *)sd->arg;

  prx_cli_lastresort(cli);
  prx_cli_reset(cli, fd);
}

// 接收来自intercept的报文
static void _loop(prx_cli_t *cli) {
  el = aeCreateEventLoop(64);

  sock_desc_t sd;
  sd.recv_notify = _on_recv;
  sd.accept = unix_svr_accept;
  sd.create_poll = unix_svr_create;
  sd.on_close = _on_close;

  int unix_fd = gen_svr_create(&sd, el, cli);

  aeMain(el);
  aeDeleteEventLoop(el);
  if (unix_fd > 0)
    close(unix_fd);
}

static void sig_int(int sig) {
  UNUSED(sig);

  if (!el)
    return;

  /*
  退出主循环
  从而正常释放cli对象
   */
  aeStop(el);
}

static void _free_splits(char **parts, uint8_t cnt) {
  for (uint8_t i = 0; i < cnt; i++) {
    free(*(parts + i));
  }
  free(parts);
}

static uint8_t _parse_host(const char *value, struct prx_cfg *cfg) {
  uint8_t out_len;
  char **parts = str_split(value, ":", &out_len);
  if (!parts)
    return 0;

  if (2 == out_len) {
    cfg->host = strdup(*parts);
    cfg->port = (uint16_t)atoi(*(parts + 1));
  }

  _free_splits(parts, out_len);
  return 1;
}

static uint8_t _parse_args(int argc, char **argv, struct prx_cfg *cfg) {
  /*
  u: unix sock path
  d: destination host & port
   */
  int opt;
  while ((opt = getopt(argc, argv, "u:d:")) != -1) {
    switch (opt) {
    case 'u':
      cfg->us_path = strdup(optarg);
      break;
    case 'd':
      if (!_parse_host(optarg, cfg))
        return 0;
      break;
    }
  }

  return cfg->port && cfg->host && cfg->us_path;
}

static void _usage(const char *exe) {
  printf("%s -u <unix_socket_path> -d <dest host>\n", exe);
}

static void _suffix(const char *us_path, char *title, uint8_t len) {
  char *last_slash = rindex(us_path, '/');
  snprintf(title, len, "%s", last_slash + 1);
}

int main(int argc, char **argv) {
  // prepare for proc title
  spt_init(argc, argv);

  struct prx_cfg cfg;
  memset(&cfg, 0, sizeof(cfg));

  if (!_parse_args(argc, argv, &cfg)) {
    _usage(*argv);
    return 0;
  }

  char title[32];
  _suffix(cfg.us_path, title, sizeof(title));
  setproctitle("prx %s", title);

  // ctrl+c
  signal(SIGINT, sig_int);
  /*
  sendmsg函数发送过程中
  对端关闭连接
  内核默认的处理流程是触发SIGPIPE消息
  导致杀死进程

  避免方法有俩
  1. 对pipe消息ignore处理；针对的是全进程
  2. 对sendmsg调用的flags参数填充 MSG_NOSIGNAL; 仅针对本地调用

  无论哪种方案
  如链路断开，如对端应答RST
  都依然会导致send/sendmsg函数返回-1,并且errno赋值
   */
  signal(SIGPIPE, SIG_IGN);

  prx_cli_t *cli = prx_cli_run(cfg.us_path, cfg.host, cfg.port);
  if (cli) {
    _loop(cli);
    prx_cli_free(&cli);
  }
  prx_ht_clean();
  free(cfg.host);
  free(cfg.us_path);

  return 0;
}
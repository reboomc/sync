#include "rmt_sock.h"
#include "anet.h"
#include "log.h"
#include "rmt_agent.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

static uint8_t _inv_ip(const char *ip, const rmt_ctx_t *ctx) {
  for (uint8_t i = 0; i < ctx->wl_len; i++) {
    if (0 == strcmp(ip, *(ctx->ip_list + i))) {
      return 0;
    }
  }
  return 1;
}

int rmt_svr_accept(void *arg, int efd) {
  rmt_agent_t *a = (rmt_agent_t *)arg;

  char ip[16];
  int cfd = anetTcpAccept(NULL, efd, ip, sizeof(ip), NULL);
  robot_log(LOG_D, "cli ip: %s", ip);

  if (_inv_ip(ip, rmt_agent_ctx(a))) {
    close(cfd);
    return 0;
  }

  anetEnableTcpNoDelay(NULL, cfd);
  return cfd;
}

int rmt_svr_create(void *arg) {
  rmt_agent_t *a = (rmt_agent_t *)arg;
  return anetTcpServer(NULL, rmt_agent_ctx(a)->listen_port, NULL, 11);
}
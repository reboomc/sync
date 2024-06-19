#include "rmt_proto.h"
#include "anet.h"
#include "fh_proto.h"
#include "gen_buf.h"
#include "gen_sock.h"
#include "log.h"
#include "rmt_agent.h"
#include "util.h"
#include "vector.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static gen_buf_t *_rmt_pb = NULL;

void rmt_proto_notify(void *arg, int fd, const uint8_t *buf, uint32_t len) {
  if (!_rmt_pb)
    _rmt_pb = gen_buf_new(300 * 1024, rmt_parse);

  rmt_agent_t *agent = (rmt_agent_t *)arg;

  linked_list_t *list = NULL;
  new_linklist(&list);

  char r = gen_buf_in(_rmt_pb, buf, len, list);
  while (1) {
    struct fh_product *p;
    if (!pop_item_count(list, 1, (void **)&p, NULL, 1))
      break;

    uint8_t response[] = {'*', p->id, 0x00, 0x00};
    memcpy(response + 2, &p->sn, 2);
    anetWrite(fd, response, 4);

    if (0 != p->len) {
      rmt_agent_append(agent, p->sn, p->buf, p->len);
    }

    free(p);
  }

  free_items(list, NULL, 0, NULL);
  if (r < 0) {
    close(fd);
    robot_log(LOG_N, "cli buf inv");
  }
}

void rmt_proto_interrupt(const sock_desc_t *sd, int fd) {
  UNUSED(sd);
  UNUSED(fd);
  gen_buf_reset(_rmt_pb);
}

void rmt_proto_clean() {
  if (_rmt_pb)
    gen_buf_free(_rmt_pb);
}
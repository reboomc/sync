#include "prx_cli.h"
#include "anet.h"
#include "cipher.h"
#include "ev.h"
#include "fh_proto.h"
#include "gen_buf.h"
#include "log.h"
#include "md5.h"
#include "net_ex.h"
#include "prx_ht.h"
#include "prx_proto.h"
#include "unix_sock.h"
#include "util.h"
#include "vector.h"
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MIN_STDC 201112L
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= MIN_STDC)
#include <stdatomic.h>
#else
void atomic_store(int *ptr, int value) {
  asm volatile("movl %1, %%eax\n\t"
               "movl %%eax, %0"
               : "=m"(*ptr)
               : "r"(value)
               : "eax");
}

int atomic_load(int *ptr) {
  int value;
  asm volatile("movl %1, %%eax\n\t"
               "movl %%eax, %0"
               : "=r"(value)
               : "m"(*ptr)
               : "eax");
  return value;
}
#endif

#define HB_PERIOD 20000
static const uint8_t remote_secret[] = AES_SECRET;
static uint8_t _recv_buf[128];

struct pack_desc {
  uint32_t len;
  uint8_t sign[16];
  uint8_t *plain;
  uint8_t *cipher;
};

struct _prx_cli {
  pthread_t pid;
  // 通过socket pair创建，用于通知data写入
  int evt_in;
  // pair的读端
  int evt_out;
  // 连接远端的fd
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= MIN_STDC)
  atomic_int cli_fd;
#else
  int cli_fd;
#endif
  uint16_t rmt_port;
  uint16_t out_hb_sn;
  uint16_t out_data_sn;
  uint16_t in_hb_sn;
  uint16_t in_data_sn;

  gen_buf_t *recv_buf;
  linked_list_t *intc_cache;
  linked_list_t *rmt_echo;
  aeEventLoop *el;
  const char *local_us;
  const char *remote_host;
  pthread_mutex_t mutex;
};

static void _cli_recv(struct aeEventLoop *el, int fd, void *arg, int mask);

static void _free_desc(void *arg) {
  struct pack_desc *d = (struct pack_desc *)arg;
  if (!d)
    return;

  if (d->plain)
    free(d->plain);
  if (d->cipher)
    free(d->cipher);
  free(d);
}

static int _try_conn(const char *host, uint16_t port) {
  int fd = 0;
  while (1) {
    fd = anetTcpConnect(NULL, host, port);
    if (fd > 0) {
      /*
      不再启用keepalive
      而仅通过业务层的心跳维护

      ka会导致cli端的FIN_WAIT2状态无法退出
       */
      anetNonBlock(NULL, fd);
      return fd;
    }

    robot_log(LOG_W, "con %s fail, retry", host);
    sleep(5);
  }
}

static void _cli_stat(prx_cli_t *cli, const uint8_t *buf, uint32_t n) {
  gen_buf_in(cli->recv_buf, buf, n, cli->rmt_echo);

  while (1) {
    struct fh_product *p;
    if (!pop_item_count(cli->rmt_echo, 1, (void **)&p, NULL, 1))
      break;

    if (0 == p->id) {
      cli->in_hb_sn = p->sn;
    } else {
      cli->in_data_sn = p->sn;
    }

    free(p);
  }
}

static void *_reconn(void *arg) {
  prx_cli_t *cli = (prx_cli_t *)arg;
  int fd = _try_conn(cli->remote_host, cli->rmt_port);
  aeCreateFileEvent(cli->el, fd, AE_READABLE, _cli_recv, cli);

  atomic_store(&cli->cli_fd, fd);
  return NULL;
}

// 收到远端读事件
static void _cli_recv(struct aeEventLoop *el, int fd, void *arg, int mask) {
  UNUSED(mask);

  prx_cli_t *cli = (prx_cli_t *)arg;
  uint32_t neg1_len;

  int n = anetRead(fd, _recv_buf, sizeof(_recv_buf), &neg1_len);

  if (0 == n) {
    atomic_store(&cli->cli_fd, -1);
    close(fd);

    robot_log(LOG_N, "closed");

    aeDeleteFileEvent(el, fd, 1);
    gen_buf_reset(cli->recv_buf);
    pthread_t pid;
    pthread_create(&pid, ((void *)0), _reconn, cli);
    return;
  }

  if (-1 == n && (EAGAIN == errno || EWOULDBLOCK == errno)) {
    n = neg1_len;
  }
  if (n > 0) {
    _cli_stat(cli, _recv_buf, n);
  }
}

static int _periodic_hb(struct aeEventLoop *el, long long id, void *arg) {
  UNUSED(el);
  UNUSED(id);

  prx_cli_t *cli = (prx_cli_t *)arg;
  int fd = atomic_load(&cli->cli_fd);
  if (fd <= 0)
    return HB_PERIOD;

  uint8_t buf[4];
  prx_proto_hb(cli->out_hb_sn++, buf);

  anetWrite(fd, buf, sizeof(buf));
  return HB_PERIOD;
}

static void *_cli_routine(void *arg) {
  prx_cli_t *cli = (prx_cli_t *)arg;

  int fd = _try_conn(cli->remote_host, cli->rmt_port);
  atomic_store(&cli->cli_fd, fd);

  aeCreateFileEvent(cli->el, fd, AE_READABLE, _cli_recv, cli);
  long long timer_id =
      aeCreateTimeEvent(cli->el, HB_PERIOD, _periodic_hb, cli, NULL);

  aeMain(cli->el);
  aeDeleteTimeEvent(cli->el, timer_id);
  aeDeleteEventLoop(cli->el);

  close(fd);
  return NULL;
}

// 通过pair通道发送的通知回调
static void _data_new(struct aeEventLoop *el, int _, void *arg, int mask) {
  UNUSED(el);
  UNUSED(_);
  UNUSED(mask);

  prx_cli_t *cli = (prx_cli_t *)arg;

  uint8_t pipe_temp[1];
  recv(cli->evt_out, pipe_temp, 1, 0);

  while (1) {
    struct pack_desc *d;
    peek_front(cli->intc_cache, (void **)&d, NULL, &cli->mutex);
    if (NULL == d) {
      // 没有可用的报文
      return;
    }

    if (!d->cipher) {
      // 明文的摘要
      md5(d->plain, d->len, d->sign);

      d->cipher = encrypt(d->plain, d->len, &d->len, remote_secret);
      free(d->plain);
      d->plain = NULL;
    }

    int fd = atomic_load(&cli->cli_fd);
    if (fd <= 0) {
      break;
    }

    ssize_t state =
        prx_send(fd, cli->out_data_sn + 1, d->cipher, d->len, d->sign);

    if (-1 == state) {
      atomic_store(&cli->cli_fd, -1);
      shutdown(fd, SHUT_WR);
      robot_log(LOG_N, "send err: %d, %s", errno, strerror(errno));
      break;
    }

    cli->out_data_sn++;
    pop_item_count(cli->intc_cache, 1, (void **)&d, &cli->mutex, 1);
    _free_desc(d);
  }
}

prx_cli_t *prx_cli_run(const char *local_us, const char *host, uint16_t port) {
  int pair[2];
  if (0 != socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) {
    return NULL;
  }

  prx_cli_t *cli = malloc(sizeof(*cli));

  cli->pid = 0;
  cli->evt_in = pair[0];
  cli->evt_out = pair[1];
  cli->local_us = local_us;
  cli->remote_host = host;
  cli->rmt_port = port;

  cli->out_hb_sn = 0;
  cli->out_data_sn = 0;
  cli->in_hb_sn = 0;
  cli->in_data_sn = 0;
  cli->recv_buf = gen_buf_new(128, prx_resp_parse);

  cli->intc_cache = NULL;
  new_linklist(&cli->intc_cache);

  cli->rmt_echo = NULL;
  new_linklist(&cli->rmt_echo);

  pthread_mutex_init(&cli->mutex, NULL);

  cli->el = aeCreateEventLoop(10);
  aeCreateFileEvent(cli->el, cli->evt_out, AE_READABLE, _data_new, cli);

  pthread_create(&cli->pid, NULL, _cli_routine, cli);
  return cli;
}

static void _notify_pair(const prx_cli_t *cli) {
  // 通知子线程发送给远端
  uint8_t signal[] = {0x00};
  ssize_t n = write(cli->evt_in, signal, 1);
  UNUSED(n);
}

void prx_cli_reset(prx_cli_t *cli, int intc_fd) {
  UNUSED(cli);
  prx_ht_reset(intc_fd);
}

void prx_cli_lastresort(const prx_cli_t *cli) { _notify_pair(cli); }

void prx_cli_notify(prx_cli_t *cli, int intc_fd, const uint8_t *in_buf,
                    uint32_t in_len) {
  if (!cli)
    return;

  linked_list_t *list = prx_ht_append(intc_fd, in_buf, in_len);

  while (list) {
    struct prxi *pi;
    if (!pop_item_count(list, 1, (void **)&pi, NULL, 1)) {
      break;
    }

    uint8_t *permanent = malloc(pi->tot);
    memcpy(permanent, pi->buf, pi->tot);

    struct pack_desc *desc = (struct pack_desc *)malloc(sizeof(*desc));
    desc->plain = permanent;
    desc->len = pi->tot;
    desc->cipher = NULL;
    *desc->sign = 0x00;

    free(pi);

    append_item(cli->intc_cache, desc, &cli->mutex);
    _notify_pair(cli);
  }
}

const char *prx_local_us(const prx_cli_t *cli) {
  return cli ? cli->local_us : NULL;
}

void prx_cli_free(prx_cli_t **pc) {
  if (!pc)
    return;

  prx_cli_t *cli = *pc;
  if (!cli)
    return;

  aeStop(cli->el);
  /*
  等待线程退出
  才能安全清理cli对象
   */
  if (0 != cli->pid) {
    _notify_pair(cli);
    pthread_join(cli->pid, NULL);
  }
  free_items(cli->intc_cache, _free_desc, 0, &cli->mutex);
  free_items(cli->rmt_echo, NULL, 0, NULL);
  gen_buf_free(cli->recv_buf);

  unix_svr_dispose(cli->local_us);
  free(cli);
}
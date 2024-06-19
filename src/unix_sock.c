#include "unix_sock.h"
#include "anet.h"
#include "prx_cli.h"
#include "util.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int unix_svr_accept(void *arg, int fd) {
  UNUSED(arg);
  return anetUnixAccept(NULL, fd);
}

int unix_svr_create(void *arg) {
  return anetUnixServer(NULL, prx_local_us((prx_cli_t *)arg), 0, 16);
}

void unix_svr_dispose(const char *us_path) {
  if (0 == access(us_path, F_OK)) {
    unlink(us_path);
  }
}
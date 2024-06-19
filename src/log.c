#include "log.h"
#include <bits/pthreadtypes.h>
#include <locale.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>

#define REDIS_MAX_LOGMSG_LEN 1024
#define LOG_FILE_FN "./m.log"

static int filter_level = LOG_D;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void set_level(uint32_t level) {
  if (level > LOG_W)
    return;

  filter_level = level;
}

void log_raw(int level, const char *msg) {
  const char *c = "DVNW";
  char buf[64];
  int rawmode = (level & REDIS_LOG_RAW);

  level &= 0xff;

  FILE *fp = fopen(LOG_FILE_FN, "a");
  if (!fp)
    return;

  if (rawmode) {
    fprintf(fp, "%s", msg);
  } else {
    int off;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    off = strftime(buf, sizeof(buf), "%d %b %H:%M:%S.", localtime(&tv.tv_sec));
    snprintf(buf + off, sizeof(buf) - off, "%03d", (int)tv.tv_usec / 1000);
    fprintf(fp, "%s %c %s\n", buf, c[level], msg);
  }
  fflush(fp);
  fclose(fp);
}

void robot_log(int level, const char *fmt, ...) {
  pthread_mutex_lock(&lock);
  if (level >= filter_level) {
    va_list ap;
    char msg[REDIS_MAX_LOGMSG_LEN];
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    log_raw(level, msg);
  }
  pthread_mutex_unlock(&lock);
}
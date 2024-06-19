#pragma once

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2

#define AE_FILE_EVENTS 1
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS | AE_TIME_EVENTS)
#define AE_DONT_WAIT 4

#define AE_NOMORE -1

/* Macros */
#define AE_NOTUSED(V) ((void)V)

struct aeEventLoop;

/* Types and data structures */
typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData,
                        int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id,
                       void *clientData);
typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop,
                                  void *clientData);
typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/* File event structure */
typedef struct aeFileEvent {
  int mask; /* one of AE_(READABLE|WRITABLE) */
  aeFileProc *rfileProc;
  aeFileProc *wfileProc;
  void *clientData;
} aeFileEvent;

/* A fired event */
typedef struct aeFiredEvent {
  int fd;
  int mask;
} aeFiredEvent;

typedef struct aeTimeEvent {
  long long id;  /* time event identifier. */
  long when_sec; /* seconds */
  long when_ms;  /* milliseconds */
  aeTimeProc *timeProc;
  aeEventFinalizerProc *finalizerProc;
  void *clientData;
  struct aeTimeEvent *next;
} aeTimeEvent;

typedef struct aeEventLoop {
  int maxfd;   /* highest file descriptor currently registered */
  int setsize; /* max number of file descriptors tracked */
  long long timeEventNextId;
  time_t lastTime;     /* Used to detect system clock skew */
  aeFileEvent *events; /* Registered events */
  aeFiredEvent *fired; /* Fired events */
  aeTimeEvent *timeEventHead;
  int stop;
  void *apidata; /* This is used for polling API specific data */
  aeBeforeSleepProc *beforesleep;
} aeEventLoop;

aeEventLoop *aeCreateEventLoop(int setsize);
void aeDeleteEventLoop(aeEventLoop *eventLoop);
void aeStop(aeEventLoop *eventLoop);
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
                      aeFileProc *proc, void *clientData);
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);
int aeGetFileEvents(aeEventLoop *eventLoop, int fd);
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
                            aeTimeProc *proc, void *clientData,
                            aeEventFinalizerProc *finalizerProc);
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
int aeWait(int fd, int mask, long long milliseconds);
void aeMain(aeEventLoop *eventLoop);
void aeSetBeforeSleepProc(aeEventLoop *eventLoop,
                          aeBeforeSleepProc *beforesleep);
int aeGetSetSize(aeEventLoop *eventLoop);
int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);

#ifdef __cplusplus
}
#endif

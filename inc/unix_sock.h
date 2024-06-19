#pragma once

int unix_svr_accept(void *, int);
int unix_svr_create(void *arg);
void unix_svr_dispose(const char *);
#pragma once

#include <stdint.h>

typedef struct _intcli intc_cli_t;
#define EXPORT __attribute__((visibility("default")))
/*
通过python的容器管理c对象
很明显行不通
 */
EXPORT intc_cli_t *intc_fetch(const char *us_path);
EXPORT void intc_dispose(void);
EXPORT void intc_send(const intc_cli_t *cli, const uint8_t *buf, uint32_t len);
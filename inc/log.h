#pragma once

#include <stdint.h>

#define LOG_D 0
#define LOG_V 1
#define LOG_N 2
#define LOG_W 3
#define REDIS_LOG_RAW (1 << 10) /* Modifier to log without timestamp */

void set_level(uint32_t level);
void robot_log(int level, const char *fmt, ...);
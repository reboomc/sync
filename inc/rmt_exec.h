#pragma once

#include "rmt_agent.h"
#include <stdint.h>

void rmt_agent_exec(const redis_cli_t *rdc_list, const rmt_ctx_t *ctx,
                    const uint8_t *buf, uint32_t len);
void rmt_agent_close();
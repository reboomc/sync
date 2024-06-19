#pragma once

#include <stdint.h>

typedef struct _prx_cli prx_cli_t;

/*
创建代理客户端,连接远端
@param local_us: 本地unix socket路径
 */
prx_cli_t *prx_cli_run(const char *local_us, const char *host, uint16_t port);
const char *prx_local_us(const prx_cli_t *cli);
/*
收到unix报文后，通知发送给远端
@param intc_fd: py端fd
 */
void prx_cli_notify(prx_cli_t *cli, int intc_fd, const uint8_t *buf,
                    uint32_t len);
// 在unix断开后，发送空报文；促使可能的残留包发出
void prx_cli_lastresort(const prx_cli_t *cli);
// 重置断开连接的buf指针
void prx_cli_reset(prx_cli_t *cli, int intc_fd);
void prx_cli_free(prx_cli_t **cli);
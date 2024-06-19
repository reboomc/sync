#pragma once

#include <stdint.h>

#define PRX_HEAD_LEN 4

/*
构造心跳报文
@param sn: 序号
@param out: 输出报文
 */
uint8_t prx_proto_hb(uint16_t sn, uint8_t *out);
/*
打包data头部4字节
 */
uint8_t prx_proto_data_header(uint16_t sn, uint8_t *out);

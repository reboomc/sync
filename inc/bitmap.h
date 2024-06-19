#pragma once

#include <stdint.h>

typedef struct _bmap bmap_t;

/*
创建相应字节长度的bmap
 */
bmap_t *bmap_new(uint16_t bytes);
void bmap_set(bmap_t *b, uint32_t pos, uint8_t on);
uint8_t bmap_get(const bmap_t *b, uint32_t pos);
void bmap_free(bmap_t *b);
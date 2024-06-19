#pragma once

#include <stdint.h>
#include <stdlib.h>

// just an example here
#define AES_SECRET                                                             \
  { 90, 42, 234, 58, 212, 8, 235, 215, 244, 11, 91, 131, 219, 255, 88, 142 }

uint8_t *encrypt(const uint8_t *plain, uint32_t len, uint32_t *target_size,
                 const uint8_t *secret);
uint8_t *decrypt(const uint8_t *cipher, size_t len, const uint8_t *secret,
                 size_t *out_len);

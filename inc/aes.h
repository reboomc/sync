#pragma once

#include <stdint.h>

void aes_enc(const uint8_t *input, const uint8_t *key, uint8_t *output,
             uint32_t length);
void aes_dec(const uint8_t *input, const uint8_t *key, uint8_t *output,
             uint32_t length);

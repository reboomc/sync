#include "cipher.h"
#include "aes.h"
#include <stdint.h>
#include <string.h>

#define ALIGN_SIZE 16
#define MD5_LEN 16

uint32_t _align(uint32_t size) {
  if (0 == size % ALIGN_SIZE)
    return size;
  return ALIGN_SIZE * (size / ALIGN_SIZE + 1);
}

// key size must be 16
uint8_t *encrypt(const uint8_t *plain, uint32_t len, uint32_t *target_size,
                 const uint8_t *secret) {
  if (0 == len) {
    return NULL;
  }

  *target_size = _align(len);
  uint8_t *cipher = malloc(*target_size);

  for (unsigned int i = 0; i < *target_size / ALIGN_SIZE; i++) {
    int offset = i * ALIGN_SIZE;

    if ((uint32_t)((i + 1) * ALIGN_SIZE) > len) {
      // size not enough
      uint8_t *temp = malloc(ALIGN_SIZE);
      int space = len - offset;
      int padding = ALIGN_SIZE - space;

      memcpy(temp, plain + offset, space);
      memset(temp + space, padding, padding);

      aes_enc(temp, secret, cipher + offset, ALIGN_SIZE);
      free(temp);
    } else {
      aes_enc(plain + offset, secret, cipher + offset, ALIGN_SIZE);
    }
  }

  return cipher;
}

static uint8_t *_decrypt_impl(const uint8_t *cipher, size_t len,
                              const uint8_t *secret) {
  if (0 != len % ALIGN_SIZE) {
    return NULL;
  }
  uint8_t *plain = malloc(len);
  uint8_t *temp = malloc(ALIGN_SIZE);

  for (uint32_t i = 0; i < len / ALIGN_SIZE; i++) {
    aes_dec(cipher + i * ALIGN_SIZE, secret, temp, ALIGN_SIZE);
    memcpy(plain + i * ALIGN_SIZE, temp, ALIGN_SIZE);
  }

  free(temp);
  return plain;
}

/*
既然是按照16字节对齐,那么最后一位必然不会超出16
if response with uint8_t, will overflow
*/
static uint32_t _padding_index(const uint8_t *txt, size_t len) {
  uint8_t last = *(txt + len - 1);
  if (last >= ALIGN_SIZE) {
    return 0;
  }
  int error = 0;
  for (size_t i = len - 2; i >= len - last; i--) {
    if (*(txt + i) == last)
      continue;

    error = 1;
    break;
  }

  if (error)
    return 0;
  return len - last;
}

uint8_t *decrypt(const uint8_t *cipher, size_t len_plain, const uint8_t *secret,
                 size_t *out_len) {
  *out_len = 0;

  uint8_t *plain = _decrypt_impl(cipher, len_plain, secret);
  if (!plain) {
    return NULL;
  }

  uint32_t idx = _padding_index(plain, len_plain);
  *out_len = idx ? idx : len_plain;

  return plain;
}
#include "prx_proto.h"
#include <stdint.h>
#include <string.h>

static uint8_t _prx_enc(uint16_t sn, uint8_t *buf, uint8_t id) {
  if (!buf)
    return 0;

  *buf = '*';
  *(buf + 1) = id;
  memcpy(buf + 2, &sn, sizeof(uint16_t));

  return 1;
}

uint8_t prx_proto_hb(uint16_t sn, uint8_t *out) { return _prx_enc(sn, out, 0); }

uint8_t prx_proto_data_header(uint16_t sn, uint8_t *out) {
  return _prx_enc(sn, out, 1);
}

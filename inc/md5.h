#pragma once

#include <stdint.h>
#include <stdlib.h>

void md5(const uint8_t *msg, size_t len, uint8_t *digest);
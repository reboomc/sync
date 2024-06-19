#include "intc_cli.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main() {
  const char *us_path = "/tmp/us1";
  char expr[] = "6379,1\x01*3\r\n$3\r\nset\r\n$1\r\na\r\n$1\r\nb\r\n";

  uint8_t buf[4];
  uint32_t len = strlen(expr);
  memcpy(buf, &len, 4);

  for (size_t i = 0; i < 100000000; i++) {
    intc_cli_t *cli = intc_fetch(us_path);

    intc_send(cli, buf, 4);
    intc_send(cli, (const uint8_t *)expr, len);
  }

  intc_dispose();
  return 0;
}

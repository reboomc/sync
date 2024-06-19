# scenarios

[中文版本文档](./readme_cn.md)<br>
To synchronize redis & mongo data between linux servers, in a fast, safe and robust way.<br>
more data formats on the way, for sure.

In local environments, two parts included:
1. c & python library interface ready for invoking.
1. packed data frame transferred to local proxy via unix-domain socket, and synchronized to remote machine, right away.

Meanwhile, in remote machine<br>
data frame received by sync_server, and written to redis & mongo database, as designed.

modules layout: 
```
|<----------local machine----------->|    |remote|
+-------------------------+    +-----+    +------+
| inner-function interface+--->+proxy+--->+server|
+-------------------------+    +-----+    +------+
```

## how to build

```bash
# mongo driver for c, see https://www.mongodb.com/docs/languages/c/c-driver/current/libmongoc/tutorial/
sudo apt install libmongoc-dev cmake libpcre3-dev
sudo pip install cython
# build python module
python setup.py build_ext -i
# build c library, proxy & sserver
cmake -Boutput
cmake --build output
```

# how to use

## 1. start server

```bash
./sync_server ./configs/rmt.ini
```

## 2. start proxy

```bash
./proxy -u <unix_socket_path> -d <dest host>
```

## 3. invoke library

```python
import intc
from intc_mg import mg_insert_many, mg_delete_many

def run():
    ic = intc.IncClient()
    us_path = '/tmp/us1'
    for i in xrange(1):
        # one redis item
        ic.redis_one(us_path, 6379, 1, ("set", "a", "bcd%d" % i))
        # multiple redis items in transaction
        ic.redis_many(
            us_path,
            6379,
            2,
            (("set", "cool", "hello%d" % i), ("set", "awesome", "中文你好%d" % i)),
        )
        # delete mongo records
        ic.mongo_exec(us_path, 'mongo_db_name', 'mongo_col_name', mg_delete_many({'key': 'value'}))

run()
```
and...

```c
#include "intc_cli.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main() {
  const char *us_path = "/tmp/us1";
  // redis expression need to build
  char expr[] = "6379,1\x01*3\r\n$3\r\nset\r\n$1\r\na\r\n$1\r\nb\r\n";

  uint8_t buf[4];
  uint32_t len = strlen(expr);
  memcpy(buf, &len, 4);

  for (size_t i = 0; i < 1; i++) {
    intc_cli_t *cli = intc_fetch(us_path);

    // send length for prefix
    intc_send(cli, buf, 4);
    // send redis frames
    intc_send(cli, (const uint8_t *)expr, len);
  }

  intc_dispose();
  return 0;
}
```

# benchmark

```bash
time python test.py
```

# autostart via systemd

```bash
sudo cp syncs.service /lib/systemd/system
sudo systemctl enable syncs.service
sudo systemctl start syncs.service
```
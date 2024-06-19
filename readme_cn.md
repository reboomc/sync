# 适用场景

用于两组服务器之间快速同步redis和mongo数据<br>
当然后续也可以加入更多其他数据格式

在本地环境下, 布局为
1. 分别以c和python库形式提供函数接口
1. 库内部通过unix-domain socket将结构化数据发送给本机代理(proxy),最终统一发往远程

而在远程环境中
1. 服务器接收来自代理的结构化数据,解析并按照配置入库;完成数据同步的目标

整体架构如下: 

```
|<----------local machine----------->|    |remote|
+-------------------------+    +-----+    +------+
| inner-function interface+--->+proxy+--->+server|
+-------------------------+    +-----+    +------+
```

## 如何编译

```bash
# mongo驱动,见 https://www.mongodb.com/docs/languages/c/c-driver/current/libmongoc/tutorial/
sudo apt install libmongoc-dev cmake libpcre3-dev
sudo pip install cython
# 编译py库
python setup.py build_ext -i
# 编译c动态库,proxy以及server
cmake -Boutput
cmake --build output
```

# 如何使用

## 1. 启动服务器

```bash
./sync_server ./configs/rmt.ini
```

## 2. 启动proxy

```bash
./proxy -u <unix_socket_path> -d <dest host>
```

## 3. 函数调用

```python
import intc
from intc_mg import mg_insert_many, mg_delete_many

def run():
    ic = intc.IncClient()
    us_path = '/tmp/us1'
    for i in xrange(1):
        # 写入单条redis记录
        ic.redis_one(us_path, 6379, 1, ("set", "a", "bcd%d" % i))
        # 在一次事务中写入多条redis记录
        ic.redis_many(
            us_path,
            6379,
            2,
            (("set", "cool", "hello%d" % i), ("set", "awesome", "中文你好%d" % i)),
        )
        # 删除指定mongo记录
        ic.mongo_exec(us_path, 'mongo_db_name', 'mongo_col_name', mg_delete_many({'key': 'value'}))

run()
```
以及

```c
#include "intc_cli.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main() {
  const char *us_path = "/tmp/us1";
  // 需自行组建redis表达式,参考./intc_cyw.pyx:_redis_wrap函数
  char expr[] = "6379,1\x01*3\r\n$3\r\nset\r\n$1\r\na\r\n$1\r\nb\r\n";

  uint8_t buf[4];
  uint32_t len = strlen(expr);
  memcpy(buf, &len, 4);

  for (size_t i = 0; i < 1; i++) {
    intc_cli_t *cli = intc_fetch(us_path);

    // 发送前缀长度
    intc_send(cli, buf, 4);
    // 发送需同步的redis报文
    intc_send(cli, (const uint8_t *)expr, len);
  }

  intc_dispose();
  return 0;
}
```

# 性能测试

```bash
time python test.py
```

# 由systemd管理自启动

```bash
sudo cp syncs.service /lib/systemd/system
sudo systemctl enable syncs.service
sudo systemctl start syncs.service
```
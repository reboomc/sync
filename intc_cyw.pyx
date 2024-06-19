from libc.stdint cimport uint8_t, uint32_t
import struct
import json

cdef extern from "intc_cli.h":
    cdef struct _intcli:
        pass

    _intcli *intc_fetch(const char *us_path)
    void intc_dispose()
    void intc_send(const _intcli *cli, const uint8_t *buf, uint32_t len)


def _redis_enc(value):
    # cite from py_redis
    if isinstance(value, (bytes, memoryview)):
        return value
    if isinstance(value, bool):
        raise ValueError('invalid type bool')
    if isinstance(value, float):
        return repr(value).encode()
    if isinstance(value, (int, long)):
        return str(value)
    return value


def head(func):
    def __(*args):
        e = func(*args)
        return struct.pack('<I', len(e)) + e
    return __


def _redis_one(cmd):
    args = []
    for i, x in enumerate(cmd):
        if 0 == i and 'delete' == x:
            x = 'del'
        x = _redis_enc(x)
        args.append('$%d\r\n%s\r\n' % (len(x), x))
    return ''.join(('*%d\r\n' % len(cmd), ''.join(args)))


write_red_cmds = {"set", "setnx", "incr", "sadd", "srem", "delete", "hset", "hsetnx", "hdel", "rpush"}


def _redis_many(cmd_list):
    l = []
    for x in cmd_list:
        if x[0] not in write_red_cmds:
            continue
        expr = _redis_one(x)
        l.append(struct.pack('<H', len(expr)) + expr)

    return len(l), ''.join(l)


@head
def _redis_wrap(port, db, func, cmd):
    expr = func(cmd)
    if isinstance(expr, (str, bytes)):
        multi = 0
        cnt = 1
    else:
        multi = 1
        cnt, expr = expr

    cnt = multi << 7 | (cnt & 0x7f)
    return '%d,%d%s%s' % (port, db, struct.pack('B', cnt), expr)


@head
def _mg_pack(db, tab, obj):
    c = {
        'db': db,
        'tab': tab,
        'o': obj,
    }
    return json.dumps(c, separators=(',', ':'))


cdef class IncClient:
    def __cinit__(self):
        pass

    def __dealloc__(self):
        intc_dispose()

    cpdef redis_one(self, us_path, port, db, cmd):
        cdef const char *c_path = us_path
        cdef _intcli* cli = intc_fetch(c_path)

        expr = _redis_wrap(port, db, _redis_one, cmd)
        cdef const uint8_t *body = expr
        intc_send(cli, body, len(expr))
    
    cpdef redis_many(self, us_path, port, db, cmd_list):
        # 没法用python语法包装c函数
        cdef const char *c_path = us_path
        cdef _intcli* cli = intc_fetch(c_path)

        expr = _redis_wrap(port, db, _redis_many, cmd_list)
        cdef const uint8_t *body = expr
        intc_send(cli, body, len(expr))

    cpdef mongo_exec(self, us_path, db, tab, obj):
        cdef const char *c_path = us_path
        cdef _intcli* cli = intc_fetch(c_path)
        # obj: 由外部打包对象
        expr = _mg_pack(db, tab, obj)
        cdef const uint8_t *body = expr
        intc_send(cli, body, len(expr))

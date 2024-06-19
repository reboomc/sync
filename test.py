# coding:utf-8


import intc


def run():
    ic = intc.IncClient()
    us_path = '/tmp/us1'
    for i in xrange(100000000):
        # red db keep same as config
        ic.redis_one(us_path, 6379, 1, ("set", "a", "bcd%d" % i))
        ic.redis_many(
            us_path,
            6379,
            2,
            (("set", "cool", "hello%d" % i), ("set", "awesome", "中文你好%d" % i)),
        )


run()

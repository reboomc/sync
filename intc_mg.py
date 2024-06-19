# coding:utf-8


def mg_insert(data):
    """
    此处均不涉及表名
    """
    if not isinstance(data, dict):
        raise ValueError("data not dict, %s" % type(data))
    return {
        "mtd": "insert",
        "v": data,
    }


def mg_insert_many(data):
    if not isinstance(data, (list, tuple)):
        raise ValueError("data not list, %s" % type(data))

    return {"mtd": "insertm", "v": data}


def mg_drop():
    return {"mtd": "drop"}


def mg_delete_many(cond):
    if not isinstance(cond, dict):
        raise ValueError("cond not dict, %s" % type(cond))
    return {"mtd": "del", "v": cond}


def _update(cond, new_kv, iine, method):
    if not isinstance(cond, dict):
        raise ValueError("cond not dict, %s" % type(cond))
    if not isinstance(new_kv, dict):
        raise ValueError("kv not dict, %s" % type(new_kv))
    if not isinstance(iine, bool):
        raise ValueError("iine not bool, %s" % type(iine))
    return {
        "mtd": method,
        "v": {
            "c": cond,
            "new": new_kv,
            "iine": iine,
        },
    }


def mg_udpate(cond, new_kv, iine):
    return _update(cond, new_kv, iine, "update")


def mg_update_many(cond, new_kv, iine):
    return _update(cond, new_kv, iine, "updatem")

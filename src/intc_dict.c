#include "dict.h"
#include "intc_cli.h"
#include "intc_con.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

// redis中运行时随机生成
#define HASH_SEED                                                              \
  { 231, 47, 235, 66, 243, 128, 47, 125, 172, 49, 236, 235, 65, 141, 64, 59 }

static dict *_d = NULL;
static close_cli _close_func = NULL;

static uint64_t _hash(const void *key) {
  static uint8_t seed[] = HASH_SEED;

  const uint8_t *in = (const uint8_t *)key;
  return siphash_nocase(in, strlen((const char *)in), seed);
}

static void *_dup(void *privdata, const void *key) {
  UNUSED(privdata);
  return strdup((const char *)key);
}

static int _cmp(void *privdata, const void *key1, const void *key2) {
  UNUSED(privdata);
  return 0 == strcmp((const char *)key1, (const char *)key2);
}

static void _free_key(void *privdata, void *key) {
  UNUSED(privdata);
  free(key);
}

static void _free_val(void *privdata, void *obj) {
  UNUSED(privdata);

  intc_cli_t *cli = (intc_cli_t *)obj;
  _close_func(cli);
}

static dictType _t = {_hash, _dup, NULL, _cmp, _free_key, _free_val, NULL};

intc_cli_t *intc_fetch_dict(const char *us_path, start_cli sf) {
  if (NULL == _d) {
    _d = dictCreate(&_t, NULL);
  }

  dictEntry *de = dictFind(_d, us_path);
  if (de) {
    return (intc_cli_t *)de->v.val;
  }

  intc_cli_t *cli = sf(us_path);
  dictAdd(_d, (void *)us_path, cli);
  return cli;
}

void intc_dispose_dict(close_cli cf) {
  _close_func = cf;
  dictRelease(_d);
}
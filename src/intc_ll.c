#include "intc_cli.h"
#include "intc_con.h"
#include "vector.h"
#include <stdio.h>
#include <string.h>

struct _wrap_cli {
  char *us_path;
  intc_cli_t *cli;
};

static linked_list_t *_cli_list = NULL;
static close_cli _close_func = NULL;

static uint8_t _match_cli(const void *item, const void *arg) {
  struct _wrap_cli *w = (struct _wrap_cli *)item;
  const char *p = (const char *)arg;

  return 0 == strcmp(w->us_path, p);
}

intc_cli_t *intc_fetch_ll(const char *us_path, start_cli sf) {
  new_linklist(&_cli_list);
  void *target = find_item(_cli_list, _match_cli, us_path, NULL);
  if (target) {
    return ((struct _wrap_cli *)target)->cli;
  }

  struct _wrap_cli *w = malloc(sizeof(*w));
  w->us_path = strdup(us_path);
  w->cli = sf(us_path);

  append_item(_cli_list, w, NULL);
  return w->cli;
}

static void _free_wrap(void *arg) {
  struct _wrap_cli *w = (struct _wrap_cli *)arg;

  free(w->us_path);
  _close_func(w->cli);
  free(w);
}

void intc_dispose_ll(close_cli cf) {
  _close_func = cf;
  free_items(_cli_list, _free_wrap, 0, NULL);
}
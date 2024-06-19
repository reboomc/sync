#pragma once

struct _intcli;

typedef struct _intcli *(*start_cli)(const char *us_path);
typedef void (*close_cli)(struct _intcli *);

struct _intcli *intc_fetch_ll(const char *us_path, start_cli sf);
void intc_dispose_ll(close_cli);

struct _intcli *intc_fetch_dict(const char *us_path, start_cli sf);
void intc_dispose_dict(close_cli);
#include "util.h"
#include <libgen.h>
#include <pcre.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define LINUX_SEP '/'

void hex_print(const uint8_t *s, uint32_t len, char *output, size_t size) {
  uint32_t offset = 0;
  for (uint32_t i = 0; i < len; i++) {
    uint8_t c = *(s + i);
    if (c >= ' ' && c <= '~') {
      if (output) {
        offset += snprintf(output + offset, size - offset, "%c", c);
      } else {
        printf("%c", c);
      }
    } else {
      if (output) {
        offset += snprintf(output + offset, size - offset, "\\x%02x", *(s + i));
      } else {
        printf("\\x%02x", *(s + i));
      }
    }
  }
  if (!output)
    printf("\n");
}

char **str_split(const char *s, const char *delim, uint8_t *out_len) {
  // todo: 该数量需要修复
#define COUNT 5
#define GEN_ONE(cnt, s)                                                        \
  snprintf(temp, sizeof(temp), "%.*s", cnt, s);                                \
  *(result + len++) = strdup(temp);

  *out_len = 0;

  char **result = (char **)malloc(sizeof(char *) * COUNT);
  uint8_t len = 0;
  char temp[32];

  while (1) {
    char *pos = strstr(s, delim);
    if (!pos)
      break;

    GEN_ONE((int)(pos - s), s)
    s = pos + 1;
  }

  GEN_ONE((int)strlen(s), s)
  *out_len = len;
  return result;
}

char **re_search(const char *pattern, const char *input, uint8_t group_count) {
  if (!pattern || !input)
    return NULL;

  const char *error;
  int error_offset;

  pcre *re = pcre_compile(pattern, 0, &error, &error_offset, NULL);
  if (!re) {
    return NULL;
  }

  int o_len = (group_count + 1) * 3;
  int ovector[o_len];
  /*
  匹配开始位置
  如有多次匹配
  很显然需更新该参数
   */
  int start = 0;
  int result;
  size_t s_len = strlen(input);

  char **macthed_groups = (char **)malloc(sizeof(char *) * group_count);

  while ((result = pcre_exec(re, NULL, input, s_len, start, 0, ovector,
                             o_len)) >= 0) {
    /*
    i从1开始
    索引0为整串匹配结果的起始
    等同python中match.group()
     */
    for (int i = 1; i < result; i++) {
      int sub_offset = ovector[2 * i];
      int sub_len = ovector[2 * i + 1] - ovector[2 * i];

      char *dest = malloc(sub_len + 1);
      snprintf(dest, sub_len + 1, "%.*s", sub_len, input + sub_offset);

      *(macthed_groups + i - 1) = dest;
    }

    // 本组匹配结果的end,作为下组的开始
    start = ovector[1];
  }

  pcre_free(re);
  return macthed_groups;
}

char *path_join(const char *root, const char *rel_path) {
  if (!root || !rel_path)
    return NULL;

  if (LINUX_SEP == *rel_path) {
    // 已经是根目录
    size_t len = strlen(rel_path) + 1;
    char *result = malloc(len);
    snprintf(result, len, "%s", rel_path);
    return result;
  }

  size_t a_len = strlen(root);
  size_t b_len = strlen(rel_path);

  size_t f_len = a_len + b_len + 2;
  char *result = malloc(f_len);

  snprintf(result, f_len, "%s", root);
  uint8_t suffix_slash = 0;
  if (LINUX_SEP != *(root + a_len - 1)) {
    *(result + a_len) = LINUX_SEP;
    suffix_slash = 1;
  }

  int left;
  const char *start;
  if (b_len > 2 && '.' == *rel_path && LINUX_SEP == *(rel_path + 1)) {
    // 以 ./ 开头
    left = (int)(b_len - 2);
    start = rel_path + 2;
  } else {
    left = (int)b_len;
    start = rel_path;
  }
  snprintf(result + a_len + suffix_slash, f_len - a_len, "%.*s", left, start);

  return result;
}

char *resolve_cwd(const char *startup_arg) {
  if (!startup_arg)
    return NULL;

  if (LINUX_SEP == *startup_arg) {
    size_t len = strlen(startup_arg);
    char *s = malloc(len + 1);
    snprintf(s, len + 1, "%s", startup_arg);
    // 绝对路径
    return dirname(s);
  }

  char cwd[256];
  char *_ = getcwd(cwd, sizeof(cwd));
  UNUSED(_);

  char *full = path_join(cwd, startup_arg);
  return dirname(full);
}
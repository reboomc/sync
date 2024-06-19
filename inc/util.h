#pragma once

#include <stddef.h>
#include <stdint.h>

#define UNUSED(x) (void)(x)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

void hex_print(const uint8_t *s, uint32_t len, char *output, size_t size);
char **str_split(const char *s, const char *delim, uint8_t *out_len);
/*
正则匹配目标字符串,确保正则匹配成功

@param pattern:正则串，由于c语法的要求，反斜杠需转义
@param input: 待匹配的输入串
@param group_count: 正则串中匹配括号的数量
 */
char **re_search(const char *pattern, const char *input, uint8_t group_count);
// join absolute root and its relative path
char *path_join(const char *root, const char *rel_path);
/*
解析工作目录
@param startup_arg 启动的exe参数
 */
char *resolve_cwd(const char *startup_arg);
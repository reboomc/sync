#ifndef UTIL_LINKLIST_H
#define UTIL_LINKLIST_H

#ifdef _WIN32
#else
#include <pthread.h>
#endif

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef _WIN32
#define pthread_mutex_t void
#endif

#ifdef __cpluscplus
extern "C" {
#endif

typedef void (*item_free)(void *item);
typedef uint8_t (*item_match)(const void *item, const void *arg);

#define DEFAULT_FREE_IMPL(type)                                                \
  if (!item) {                                                                 \
    return;                                                                    \
  }                                                                            \
  type *node = (type *)item;                                                   \
  if (!node) {                                                                 \
    return;                                                                    \
  }                                                                            \
  free(node);

typedef struct _link_head linked_list_t;

uint8_t new_linklist(linked_list_t **head);

/*
head节点保持不变，且置空
追加head之后的指针

@return 成功则返回唯一序号
 */
uint32_t append_item(linked_list_t *head, void *item, pthread_mutex_t *lock);
/*
拷贝至目标指针内存中
纠正真实总量
 */
size_t pop_item_count(linked_list_t *head, size_t total, void **items,
                      pthread_mutex_t *lock, uint8_t must);
void peek_front(linked_list_t *head, void **target, uint32_t *seq,
                pthread_mutex_t *lock);
void *find_item(linked_list_t *head, item_match im, const void *arg,
                pthread_mutex_t *lock);
/*
通过下标访问
并且支持负数
 */
void *index_item(const linked_list_t *head, ssize_t index,
                 pthread_mutex_t *lock);

// 当前计数
size_t count_items(const linked_list_t *head, pthread_mutex_t *lock);
void *iter_copy(linked_list_t *head, size_t item_size, uint32_t *len,
                pthread_mutex_t *lock);
/*
需要释放节点的实际内存
caller必须提供有效的释放实现

@param keep_head 保留头部，后续还可继续追加
*/
void free_items(linked_list_t *head, item_free ef, uint8_t keep_head,
                pthread_mutex_t *lock);

#ifdef __cpluscplus
}
#endif

#endif
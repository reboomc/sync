#include "vector.h"
#ifdef _WIN32
#else
#include <pthread.h>
#endif

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct _link_item {
  /*
  用于表示唯一标志
  供调用方超时管理用
   */
  uint32_t seq;
  void *item;
  struct _link_item *next;
} impl_t;

struct _link_head {
  /*
  在头部存储数量
  以及锁
   */
  uint32_t count;
  uint32_t base_seq;
  struct _link_item *begin;
};

static void _do_lock(pthread_mutex_t *m) {
  if (!m) {
    return;
  }

#ifdef __gnu_linux__
  pthread_mutex_lock(m);
#endif
}

static void _unlock(pthread_mutex_t *m) {
  if (!m) {
    return;
  }

#ifdef __gnu_linux__
  pthread_mutex_unlock(m);
#endif
}

uint8_t new_linklist(linked_list_t **head) {
  if (!head) {
    return 0;
  }
  if (NULL != *head) {
    return 1;
  }

  linked_list_t *n = (linked_list_t *)malloc(sizeof(linked_list_t));
  if (!n) {
    *head = NULL;
    return 0;
  }

  n->base_seq = 1;
  n->count = 0;
  n->begin = NULL;

  *head = n;
  return 1;
}

uint32_t append_item(linked_list_t *head, void *item, pthread_mutex_t *lock) {
  if (!head || !item) {
    return 0;
  }

  impl_t *new = (impl_t *)malloc(sizeof(impl_t));
  if (!new) {
    return 0;
  }

  _do_lock(lock);

  // init
  new->item = item;
  new->next = NULL;
  new->seq = ++head->base_seq;

  if (!head->begin) {
    head->begin = new;
    head->count += 1;

    _unlock(lock);
    return new->seq;
  }

  impl_t *x = head->begin;

  while (1) {
    if (NULL == x->next) {
      break;
    }
    x = x->next;
  }

  x->next = new;
  head->count += 1;

  _unlock(lock);
  return new->seq;
}

size_t count_items(const linked_list_t *head, pthread_mutex_t *lock) {
  if (!head) {
    return 0;
  }

  size_t c;
  _do_lock(lock);

  c = (size_t)head->count;

  _unlock(lock);
  return c;
}

void *index_item(const linked_list_t *head, ssize_t index,
                 pthread_mutex_t *lock) {
  if (!head) {
    return NULL;
  }

  _do_lock(lock);
  if ((index >= 0 && index >= head->count) ||
      (index < 0 && -index > head->count)) {
    _unlock(lock);
    return NULL;
  }

  if (index < 0) {
    index = head->count + index;
  }

  /*
  由于节点不是连续内存
  不能直接偏移访问
  */
  impl_t *cur = head->begin;
  for (size_t j = 0; j < (size_t)index; j++) {
    cur = cur->next;
  }

  void *t = cur->item;
  _unlock(lock);
  return t;
}

void peek_front(linked_list_t *head, void **target, uint32_t *seq,
                pthread_mutex_t *lock) {
  if (seq)
    *seq = 0;
  if (!head || !target) {
    return;
  }

  _do_lock(lock);

  impl_t *cur = head->begin;
  if (cur) {
    *target = cur->item;
    if (seq) {
      *seq = cur->seq;
    }
  } else {
    *target = NULL;
  }

  _unlock(lock);
}

size_t pop_item_count(linked_list_t *head, size_t total, void **items,
                      pthread_mutex_t *lock, uint8_t must) {
  if (!head || !total || !items) {
    return 0;
  }

  _do_lock(lock);

  if (must && head->count < total) {
    _unlock(lock);
    return 0;
  }

  size_t actual = 0;
  impl_t *iter = head->begin;
  impl_t *prior = NULL;

  while (1) {
    if (!iter || actual >= total) {
      break;
    }

    *(items + actual) = iter->item;
    actual++;

    prior = iter;
    iter = iter->next;

    // 已经取出的item，需要销毁wrap
    free(prior);
    // 修正指针
    head->begin = iter;
    head->count--;
  }

  _unlock(lock);
  return actual;
}

void *iter_copy(linked_list_t *head, size_t item_size, uint32_t *len,
                pthread_mutex_t *lock) {
  *len = 0;
  if (!head || 0 == item_size) {
    return NULL;
  }

  _do_lock(lock);

  uint32_t count = head->count;
  if (0 == count) {
    _unlock(lock);
    return NULL;
  }

  /*
  由于没有类型信息
  退化为1字节指针
   */
  char *target_list = (char *)malloc(item_size * count);
  if (!target_list) {
    _unlock(lock);
    return NULL;
  }

  impl_t *cur = head->begin;
  for (uint32_t i = 0; i < count; i++) {
    // 偏移量须设置正确
    memcpy(target_list + i * item_size, cur->item, item_size);
    cur = cur->next;
  }

  *len = count;

  _unlock(lock);
  return target_list;
}

void *find_item(linked_list_t *head, item_match im, const void *arg,
                pthread_mutex_t *lock) {
  if (!head || !im) {
    return NULL;
  }

  _do_lock(lock);

  void *target = NULL;
  impl_t *cur = head->begin;

  while (cur) {
    if (im(cur->item, arg)) {
      target = cur->item;
      break;
    }
    cur = cur->next;
  }

  _unlock(lock);
  return target;
}

void free_items(linked_list_t *head, item_free ef, uint8_t keep_head,
                pthread_mutex_t *lock) {
  if (!head) {
    return;
  }

  _do_lock(lock);

  impl_t *cur = head->begin;
  if (!cur) {
    if (keep_head) {
      head->begin = NULL;
      head->base_seq = 0;
      head->count = 0;
    } else {
      free(head);
    }

    _unlock(lock);
    return;
  }

  impl_t *next = cur->next;

  while (cur) {
    if (cur->item) {
      if (ef) {
        ef(cur->item);
      } else {
        free(cur->item);
      }
    }

    free(cur);

    cur = next;
    if (cur) {
      next = cur->next;
    } else {
      next = NULL;
    }
  }

  if (keep_head) {
    head->begin = NULL;
    head->base_seq = 0;
    head->count = 0;
  } else {
    free(head);
  }

  _unlock(lock);
}
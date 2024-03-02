/**
 * @file udict.c
 * @brief 数据类型通用的动态字典实现
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-01-23
 *
 * THINK DIFFERENTLY
 */

#include "udict.h"

#include <string.h>

#include "log.h"

#define _udict_memmove memmove
#define _udict_memcpy memcpy
#define _udict_memset memset
#define _udict_malloc m_alloc
#define _udict_free m_free
#define _udict_realloc m_realloc
#define _udict_memcmp memcmp

#if MOD_CFG_USE_OS
#define UDICT_LOCK()                                    \
  {                                                     \
    if (!dict->mutex) dict->mutex = MOD_MUTEX_CREATE(); \
    MOD_MUTEX_ACQUIRE(dict->mutex);                     \
  }
#define UDICT_UNLOCK() \
  { MOD_MUTEX_RELEASE(dict->mutex); }
#else
#define UDICT_LOCK() ((void)0)
#define UDICT_UNLOCK() ((void)0)
#endif
#define UDICT_UNLOCK_RET(x) \
  {                         \
    UDICT_UNLOCK();         \
    return x;               \
  }

static inline uint8_t fast_strcmp(const char* str1, const char* str2) {
  if (str1 == str2) return 1;
  while ((*str1) && (*str2)) {
    if ((*str1++) != (*str2++)) return 0;
  }
  return (!*str1) && (!*str2);
}

static inline char* make_key(const char* key) {
  char* k = _udict_malloc(strlen(key) + 1);
  strcpy(k, key);
  return k;
}

static udict_node_t* udict_find_node(UDICT dict, const char* key) {
  if (!dict || !key || !dict->size) {
    return NULL;
  }
  ulist_foreach(&dict->nodes, udict_node_t, node) {
    if (fast_strcmp(node->key, key)) {
      return node;
    }
  }
  return NULL;
}

static void udict_elfree(void* ptr) {
  udict_node_t* node = (udict_node_t*)ptr;
  if (node->dynamic_value) {
    _udict_free(node->value);
  }
  _udict_free((void*)node->key);
}

bool udict_init(UDICT dict) {
  if (!ulist_init(&dict->nodes, sizeof(udict_node_t), 0, ULIST_CFG_NO_MUTEX,
                  udict_elfree)) {
    return false;
  }
  dict->size = 0;
  dict->iter = 0;
  dict->dyn = false;
#if MOD_CFG_USE_OS
  dict->mutex = MOD_MUTEX_CREATE();
#endif
  return true;
}

UDICT udict_new(void) {
  UDICT dict = (UDICT)_udict_malloc(sizeof(udict_t));
  if (!dict) {
    return NULL;
  }
  if (!udict_init(dict)) {
    _udict_free(dict);
    return NULL;
  }
  dict->dyn = true;
  return dict;
}

void udict_clear(UDICT dict) {
  UDICT_LOCK();
  ulist_clear(&dict->nodes);
  dict->size = 0;
  UDICT_UNLOCK();
}

void udict_free(UDICT dict) {
  ulist_free(&dict->nodes);
#if MOD_CFG_USE_OS
  if (dict->mutex) MOD_MUTEX_FREE(dict->mutex);
#endif
  if (dict->dyn) _udict_free(dict);
}

bool udict_has_key(UDICT dict, const char* key) {
  return udict_find_node(dict, key) != NULL;
}

void* udict_get(UDICT dict, const char* key) {
  UDICT_LOCK();
  udict_node_t* node = udict_find_node(dict, key);
  UDICT_UNLOCK_RET(node ? node->value : NULL);
}

const char* udict_get_reverse(UDICT dict, void* value) {
  if (!dict || !value || !dict->size) {
    return NULL;
  }
  UDICT_LOCK();
  ulist_foreach(&dict->nodes, udict_node_t, node) {
    if (node->value == value) {
      UDICT_UNLOCK_RET(node->key);
    }
  }
  UDICT_UNLOCK_RET(NULL);
}

static inline void udict_internal_modify(udict_node_t* node, void* value,
                                         uint8_t dyn) {
  if (node->dynamic_value) _udict_free(node->value);
  node->value = value;
  node->dynamic_value = dyn;
}

bool udict_set(UDICT dict, const char* key, void* value) {
  UDICT_LOCK();
  udict_node_t* node = udict_find_node(dict, key);
  if (node) {
    udict_internal_modify(node, value, 0);
    UDICT_UNLOCK_RET(true);
  } else {
    udict_node_t* node = (udict_node_t*)ulist_append(&dict->nodes);
    if (!node) UDICT_UNLOCK_RET(false);
    node->key = make_key(key);
    node->value = value;
    node->dynamic_value = 0;
    dict->size++;
    UDICT_UNLOCK_RET(true);
  }
}

bool udict_set_copy(UDICT dict, const char* key, void* value, size_t size) {
  void* buf = _udict_malloc(size);
  if (!buf) return false;
  _udict_memcpy(buf, value, size);
  UDICT_LOCK();
  udict_node_t* node = udict_find_node(dict, key);
  if (node) {
    udict_internal_modify(node, buf, 1);
    UDICT_UNLOCK_RET(true);
  } else {
    udict_node_t* node = (udict_node_t*)ulist_append(&dict->nodes);
    if (!node) {
      _udict_free(buf);
      UDICT_UNLOCK_RET(false);
    }
    node->key = make_key(key);
    node->value = buf;
    node->dynamic_value = 1;
    dict->size++;
    UDICT_UNLOCK_RET(true);
  }
}

void* udict_set_alloc(UDICT dict, const char* key, size_t size) {
  void* buf = _udict_malloc(size);
  if (!buf) return NULL;
  UDICT_LOCK();
  udict_node_t* node = udict_find_node(dict, key);
  if (node) {
    udict_internal_modify(node, buf, 1);
    UDICT_UNLOCK_RET(buf);
  } else {
    udict_node_t* node = (udict_node_t*)ulist_append(&dict->nodes);
    if (!node) {
      _udict_free(buf);
      UDICT_UNLOCK_RET(false);
    }
    node->key = make_key(key);
    node->value = buf;
    node->dynamic_value = 1;
    dict->size++;
    UDICT_UNLOCK_RET(buf);
  }
}

bool udict_delete(UDICT dict, const char* key) {
  UDICT_LOCK();
  udict_node_t* node = udict_find_node(dict, key);
  if (!node) UDICT_UNLOCK_RET(false);
  if (node->dynamic_value) {
    _udict_free(node->value);
  }
  ulist_remove(&dict->nodes, node);
  dict->size--;
  UDICT_UNLOCK_RET(true);
}

void* udict_pop(UDICT dict, const char* key) {
  UDICT_LOCK();
  udict_node_t* node = udict_find_node(dict, key);
  if (!node) UDICT_UNLOCK_RET(NULL);
  if (node->dynamic_value) UDICT_UNLOCK_RET(NULL);
  void* value = node->value;
  ulist_remove(&dict->nodes, node);
  dict->size--;
  UDICT_UNLOCK_RET(value);
}

bool udict_iter(UDICT dict, const char** key, void** value) {
  if (!dict || !dict->size) {
    return false;
  }
  if (dict->iter >= dict->size) {
    dict->iter = 0;
    if (key) *key = NULL;
    if (value) *value = NULL;
    return false;
  }
  udict_node_t* node = (udict_node_t*)ulist_get(&dict->nodes, dict->iter);
  if (key) *key = node->key;
  if (value) *value = node->value;
  dict->iter++;
  return true;
}

void udict_iter_stop(UDICT dict) { dict->iter = 0; }

void udict_print(UDICT dict, const char* name) {
  if (!dict || !dict->size) {
    return;
  }
  LOG_RAWLN("dict(%s) = {", name);
  ulist_foreach(&dict->nodes, udict_node_t, node) {
    LOG_RAWLN("  %s: %p,", node->key, node->value);
  }
  LOG_RAWLN("}");
}

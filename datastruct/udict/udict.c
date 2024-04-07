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

#define UDICT_LOCK()                                           \
  {                                                            \
    if (!dict->mutex) dict->mutex = MOD_MUTEX_CREATE("udict"); \
    MOD_MUTEX_ACQUIRE(dict->mutex);                            \
  }
#define UDICT_UNLOCK() \
  { MOD_MUTEX_RELEASE(dict->mutex); }
#define UDICT_UNLOCK_RET(x) \
  {                         \
    UDICT_UNLOCK();         \
    return x;               \
  }

static udict_node_t* add_node(UDICT dict, const char* key) {
  udict_node_t* node =
      (udict_node_t*)_udict_malloc(sizeof(udict_node_t) + strlen(key) + 1);
  if (!node) {
    return NULL;
  }
  node->key = (const char*)(node + 1);
  strcpy((char*)node->key, key);
  HASH_ADD_KEYPTR(hh, dict->nodes, node->key, strlen(node->key), node);
  dict->size++;
  return node;
}

static void del_node(UDICT dict, void* ptr) {
  udict_node_t* node = (udict_node_t*)ptr;
  HASH_DEL(dict->nodes, node);
  if (node->dyn) {
    _udict_free(node->value);
  }
  _udict_free(node);
  dict->size--;
}

static udict_node_t* find_node(UDICT dict, const char* key) {
  if (!dict || !key || !dict->size) {
    return NULL;
  }
  udict_node_t* node = NULL;
  HASH_FIND_STR(dict->nodes, key, node);
  return node;
}

bool udict_init(UDICT dict) {
  dict->nodes = NULL;
  dict->size = 0;
  dict->dyn = false;
  dict->mutex = MOD_MUTEX_CREATE("udict");
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
  if (!dict) {
    return;
  }
  udict_node_t *node, *tmp;
  UDICT_LOCK();
  HASH_ITER(hh, dict->nodes, node, tmp) { del_node(dict, node); }
  dict->size = 0;
  UDICT_UNLOCK();
}

void udict_free(UDICT dict) {
  if (!dict) {
    return;
  }
  udict_clear(dict);
  if (dict->mutex) MOD_MUTEX_DELETE(dict->mutex);
  if (dict->dyn) _udict_free(dict);
}

bool udict_has_key(UDICT dict, const char* key) {
  return udict_get(dict, key) != NULL;
}

void* udict_get(UDICT dict, const char* key) {
  UDICT_LOCK();
  udict_node_t* node = find_node(dict, key);
  UDICT_UNLOCK_RET(node ? node->value : NULL);
}

const char* udict_get_reverse(UDICT dict, void* value) {
  if (!dict || !value || !dict->size) {
    return NULL;
  }
  udict_node_t *node, *tmp;
  UDICT_LOCK();
  HASH_ITER(hh, dict->nodes, node, tmp) {
    if (node->value == value) {
      UDICT_UNLOCK_RET(node->key);
    }
  }
  UDICT_UNLOCK_RET(NULL);
}

static inline void modify_node_value(udict_node_t* node, void* value,
                                     bool dyn) {
  if (node->dyn) _udict_free(node->value);
  node->value = value;
  node->dyn = dyn;
}

bool udict_set(UDICT dict, const char* key, void* value) {
  udict_node_t* node;
  UDICT_LOCK();
  node = find_node(dict, key);
  if (node) {
    modify_node_value(node, value, false);
    UDICT_UNLOCK_RET(true);
  } else {
    node = add_node(dict, key);
    if (!node) UDICT_UNLOCK_RET(false);
    node->value = value;
    node->dyn = false;
    UDICT_UNLOCK_RET(true);
  }
}

bool udict_set_copy(UDICT dict, const char* key, void* value, size_t size) {
  udict_node_t* node;
  void* buf = _udict_malloc(size);
  if (!buf) return false;
  _udict_memcpy(buf, value, size);
  UDICT_LOCK();
  node = find_node(dict, key);
  if (node) {
    modify_node_value(node, buf, true);
    UDICT_UNLOCK_RET(true);
  } else {
    node = add_node(dict, key);
    if (!node) {
      _udict_free(buf);
      UDICT_UNLOCK_RET(false);
    }
    node->value = buf;
    node->dyn = true;
    UDICT_UNLOCK_RET(true);
  }
}

void* udict_set_alloc(UDICT dict, const char* key, size_t size) {
  udict_node_t* node;
  void* buf = _udict_malloc(size);
  if (!buf) return NULL;
  UDICT_LOCK();
  node = find_node(dict, key);
  if (node) {
    modify_node_value(node, buf, true);
    UDICT_UNLOCK_RET(buf);
  } else {
    node = add_node(dict, key);
    if (!node) {
      _udict_free(buf);
      UDICT_UNLOCK_RET(NULL);
    }
    node->value = buf;
    node->dyn = true;
    UDICT_UNLOCK_RET(buf);
  }
}

bool udict_delete(UDICT dict, const char* key) {
  UDICT_LOCK();
  udict_node_t* node = find_node(dict, key);
  if (!node) UDICT_UNLOCK_RET(false);
  del_node(dict, node);
  UDICT_UNLOCK_RET(true);
}

void* udict_pop(UDICT dict, const char* key) {
  UDICT_LOCK();
  udict_node_t* node = find_node(dict, key);
  if (!node) UDICT_UNLOCK_RET(NULL);
  void* value = node->value;
  node->dyn = false;  // 防止释放
  del_node(dict, node);
  UDICT_UNLOCK_RET(value);
}

bool udict_iter(UDICT dict, const char** key, void** value) {
  if (!dict || !dict->size || !dict->nodes) {
    return false;
  }
  if (*key == NULL) {
    *key = dict->nodes->key;
    *value = dict->nodes->value;
    return true;
  } else {
    udict_node_t* node = find_node(dict, *key);
    if (!node) {
      return false;
    }
    node = node->hh.next;
    if (!node) {
      return false;
    }
    *key = node->key;
    *value = node->value;
    return true;
  }
}

void udict_print(UDICT dict, const char* name) {
  if (!dict || !dict->size) {
    return;
  }
  PRINTLN("dict(%s) = {", name);
  udict_node_t *node, *tmp;
  HASH_ITER(hh, dict->nodes, node, tmp) {
    PRINTLN("  %s: %p,", node->key, node->value);
  }
  PRINTLN("}");
}

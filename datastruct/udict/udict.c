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

#include "uthash.h"

#define LOG_MODULE "udict"
#include "log.h"

struct udict_node {
  UT_hash_handle hh;  // hashable
  bool is_ptr;
  mod_size_t size;
  const char* key;
  uint8_t value[];
};

typedef struct udict_node udict_node_t;

struct udict {
  udict_node_t* nodes;
  mod_size_t size;
  MOD_MUTEX_HANDLE mutex;  // 互斥锁
  bool dyn;
};

typedef struct udict udict_t;

#define _udict_malloc m_alloc
#define _udict_free m_free
#define _udict_realloc m_realloc

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

static udict_node_t* add_node(UDICT dict, const char* key, size_t value_size,
                              void* value, bool is_ptr) {
  udict_node_t* node = (udict_node_t*)_udict_malloc(
      sizeof(udict_node_t) + strlen(key) + 1 + value_size);
  if (!node) {
    return NULL;
  }
  node->size = value_size;
  node->key = (const char*)((uint8_t*)(node + 1) + value_size);
  node->is_ptr = is_ptr;
  strcpy((char*)node->key, key);
  if (value) memcpy(node->value, value, value_size);
  HASH_ADD_KEYPTR(hh, dict->nodes, node->key, strlen(node->key), node);
  dict->size++;
  return node;
}

static bool modify_node(UDICT dict, udict_node_t* node, size_t value_size,
                        void* value, bool is_ptr) {
  if (value_size <= node->size) {
    if (value) memcpy(node->value, value, value_size);
    return true;
  }
  bool ret = false;
  HASH_DEL(dict->nodes, node);
  udict_node_t* new_node = (udict_node_t*)_udict_realloc(
      node, sizeof(udict_node_t) + strlen(node->key) + 1 + value_size);
  if (new_node) {
    new_node->size = value_size;
    node = new_node;
    ret = true;
  }
  HASH_ADD_KEYPTR(hh, dict->nodes, node->key, strlen(node->key), node);
  return ret;
}

static void del_node(UDICT dict, udict_node_t* node) {
  HASH_DEL(dict->nodes, node);
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

mod_size_t udict_len(UDICT dict) { return dict->size; }

bool udict_has_key(UDICT dict, const char* key) {
  return udict_get(dict, key) != NULL;
}

void* udict_get(UDICT dict, const char* key) {
  void* ret = NULL;
  UDICT_LOCK();
  udict_node_t* node = find_node(dict, key);
  if (node) {
    if (node->is_ptr) {
      ret = *(void**)node->value;
    } else {
      ret = node->value;
    }
  }
  UDICT_UNLOCK_RET(ret);
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

bool udict_set(UDICT dict, const char* key, void* value) {
  udict_node_t* node;
  UDICT_LOCK();
  node = find_node(dict, key);
  if (node) {
    bool ret = modify_node(dict, node, sizeof(void*), (void*)(&value), true);
    UDICT_UNLOCK_RET(ret);
  } else {
    node = add_node(dict, key, sizeof(void*), (void*)(&value), true);
    UDICT_UNLOCK_RET(node ? true : false);
  }
}

bool udict_set_copy(UDICT dict, const char* key, void* value, size_t size) {
  udict_node_t* node;
  UDICT_LOCK();
  node = find_node(dict, key);
  if (node) {
    bool ret = modify_node(dict, node, size, value, false);
    UDICT_UNLOCK_RET(ret);
  } else {
    node = add_node(dict, key, size, value, false);
    UDICT_UNLOCK_RET(node ? true : false);
  }
}

void* udict_set_alloc(UDICT dict, const char* key, size_t size) {
  udict_node_t* node;
  UDICT_LOCK();
  node = find_node(dict, key);
  if (node) {
    if (!modify_node(dict, node, size, NULL, false)) UDICT_UNLOCK_RET(NULL);
    UDICT_UNLOCK_RET(node->value);
  } else {
    node = add_node(dict, key, size, NULL, false);
    if (!node) UDICT_UNLOCK_RET(NULL);
    UDICT_UNLOCK_RET(node->value);
  }
}

bool udict_del(UDICT dict, const char* key) {
  UDICT_LOCK();
  udict_node_t* node = find_node(dict, key);
  if (!node) UDICT_UNLOCK_RET(false);
  del_node(dict, node);
  UDICT_UNLOCK_RET(true);
}

void* udict_pop(UDICT dict, const char* key) {
  void* ret = NULL;
  UDICT_LOCK();
  udict_node_t* node = find_node(dict, key);
  if (!node) UDICT_UNLOCK_RET(NULL);
  if (node->is_ptr) {
    ret = *(void**)node->value;
  } else {
    ret = _udict_malloc(node->size);
    if (!ret) UDICT_UNLOCK_RET(NULL);
    memcpy(ret, node->value, node->size);
  }
  del_node(dict, node);
  UDICT_UNLOCK_RET(ret);
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
    if (node->is_ptr) {
      *value = *(void**)node->value;
    } else {
      *value = node->value;
    }
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

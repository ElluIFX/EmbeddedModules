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

static void udict_elfree(void* ptr) {
  udict_node_t* node = (udict_node_t*)ptr;
  if (node->dynamic_value) {
    _udict_free(node->value);
  }
  _udict_free((void*)node->key);
}

static inline uint8_t fast_strcmp(const char* str1, const char* str2) {
  if (str1 == str2) return 1;
  while ((*str1) && (*str2)) {
    if ((*str1++) != (*str2++)) return 0;
  }
  return (!*str1) && (!*str2);
}

static inline const char* make_key(const char* key) {
  char* k = _udict_malloc(strlen(key) + 1);
  strcpy(k, key);
  return (const char*)k;
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

bool udict_init(UDICT dict) {
  if (!ulist_init(&dict->nodes, sizeof(udict_node_t), 0, NULL, udict_elfree)) {
    return false;
  }
  dict->size = 0;
  dict->iter = 0;
  return true;
}

UDICT udict_create(void) {
  UDICT dict = (UDICT)_udict_malloc(sizeof(udict_t));
  if (!dict) {
    return NULL;
  }
  if (!udict_init(dict)) {
    _udict_free(dict);
    return NULL;
  }
  return dict;
}

void udict_clear(UDICT dict) {
  ulist_clear(&dict->nodes);
  dict->size = 0;
}

void udict_free(UDICT dict) {
  udict_clear(dict);
  _udict_free(dict);
}

bool udict_has_key(UDICT dict, const char* key) {
  return udict_find_node(dict, key) != NULL;
}

void* udict_get(UDICT dict, const char* key) {
  udict_node_t* node = udict_find_node(dict, key);
  return node ? node->value : NULL;
}

const char* udict_get_reverse(UDICT dict, void* value) {
  if (!dict || !value || !dict->size) {
    return NULL;
  }
  ulist_foreach(&dict->nodes, udict_node_t, node) {
    if (node->value == value) {
      return node->key;
    }
  }
  return NULL;
}

static inline void udict_internal_modify(udict_node_t* node, void* value,
                                         uint8_t dyn) {
  if (node->dynamic_value) _udict_free(node->value);
  node->value = value;
  node->dynamic_value = dyn;
}

bool udict_set(UDICT dict, const char* key, void* value) {
  udict_node_t* node = udict_find_node(dict, key);
  if (node) {
    udict_internal_modify(node, value, 0);
    return true;
  } else {
    udict_node_t* node = (udict_node_t*)ulist_append(&dict->nodes);
    if (!node) return false;
    node->key = make_key(key);
    node->value = value;
    node->dynamic_value = 0;
    dict->size++;
    return true;
  }
}

bool udict_set_copy(UDICT dict, const char* key, void* value, size_t size) {
  void* buf = _udict_malloc(size);
  _udict_memcpy(buf, value, size);
  if (!buf) return false;
  udict_node_t* node = udict_find_node(dict, key);
  if (node) {
    udict_internal_modify(node, buf, 1);
    return true;
  } else {
    udict_node_t* node = (udict_node_t*)ulist_append(&dict->nodes);
    if (!node) {
      _udict_free(buf);
      return false;
    }
    node->key = make_key(key);
    node->value = buf;
    node->dynamic_value = 1;
    dict->size++;
    return true;
  }
}

void* udict_set_alloc(UDICT dict, const char* key, size_t size) {
  void* buf = _udict_malloc(size);
  if (!buf) return NULL;
  udict_node_t* node = udict_find_node(dict, key);
  if (node) {
    udict_internal_modify(node, buf, 1);
    return buf;
  } else {
    udict_node_t* node = (udict_node_t*)ulist_append(&dict->nodes);
    if (!node) {
      _udict_free(buf);
      return false;
    }
    node->key = make_key(key);
    node->value = buf;
    node->dynamic_value = 1;
    dict->size++;
    return buf;
  }
}

bool udict_delete(UDICT dict, const char* key) {
  udict_node_t* node = udict_find_node(dict, key);
  if (!node) return false;
  if (node->dynamic_value) {
    _udict_free(node->value);
  }
  ulist_remove(&dict->nodes, node);
  dict->size--;
  return true;
}

void* udict_pop(UDICT dict, const char* key) {
  udict_node_t* node = udict_find_node(dict, key);
  if (!node) return NULL;
  if (node->dynamic_value) return NULL;
  void* value = node->value;
  ulist_remove(&dict->nodes, node);
  dict->size--;
  return value;
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

void udict_print(UDICT dict) {
  if (!dict || !dict->size) {
    return;
  }
  LOG_RAWLN("dict = {");
  ulist_foreach(&dict->nodes, udict_node_t, node) {
    LOG_RAWLN("  %s: 0x%p", node->key, node->value);
  }
  LOG_RAWLN("}");
}

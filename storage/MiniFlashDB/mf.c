#include "mf.h"

#if __has_include("mf_hal.h")
#include "mf_hal.h"
#else
#include "mf_hal_template.h"
#endif

#include <stdint.h>

#define MF_FLASH_HEADER 0x3366CCFF  // 数据库头(32b)
#define MF_FLASH_TAIL 0x3E          // 数据库尾(8b)

static uint8_t mf_temp[MF_FLASH_BLOCK_SIZE];
static mf_flash_info_t *mf_data = (mf_flash_info_t *)mf_temp;
static mf_flash_info_t *info_main = NULL;
#ifdef MF_FLASH_BACKUP_ADDR
static mf_flash_info_t *info_backup = NULL;
#endif

static void mf_init_block(mf_flash_info_t *block) {
  mf_erase((uint32_t)block);
  mf_write((uint32_t)block, mf_temp);
}

static bool mf_block_inited(mf_flash_info_t *block) {
  return block->header == MF_FLASH_HEADER;
}

static bool mf_block_empty(mf_flash_info_t *block) {
  return block->key.name_length == 0;
}

static bool mf_block_err(mf_flash_info_t *block) {
  return ((uint8_t *)block)[MF_FLASH_BLOCK_SIZE - 1] != MF_FLASH_TAIL ||
         block->header != MF_FLASH_HEADER ||
         block->key.name_length > MF_FLASH_BLOCK_SIZE ||
         block->key.data_size > MF_FLASH_BLOCK_SIZE;
}

void mf_init() {
  info_main = (mf_flash_info_t *)MF_FLASH_MAIN_ADDR;
#ifdef MF_FLASH_BACKUP_ADDR
  info_backup = (mf_flash_info_t *)MF_FLASH_BACKUP_ADDR;
#endif

  mf_flash_info_t info = {
      .header = MF_FLASH_HEADER,
      .key = {.next_key = false, .name_length = 0, .data_size = 0}};

  memset(mf_temp, -1, MF_FLASH_BLOCK_SIZE);
  memcpy(mf_temp, &info, sizeof(info));
  mf_temp[MF_FLASH_BLOCK_SIZE - 1] = MF_FLASH_TAIL;

#ifdef MF_FLASH_BACKUP_ADDR
  if (!mf_block_inited(info_backup) || mf_block_err(info_backup)) {
    mf_init_block(info_backup);
  }
#endif

  if (!mf_block_inited(info_main)) {
    mf_init_block(info_main);
  }

  if (mf_block_err(info_main)) {
#ifdef MF_FLASH_BACKUP_ADDR
    if (mf_block_empty(info_backup)) {
      mf_init_block(info_main);
    } else {
      mf_write((uint32_t)info_main, info_backup);
    }
#else
    mf_init_block(info_main);
#endif
  }

  memcpy(mf_temp, info_main, MF_FLASH_BLOCK_SIZE);
}

void mf_save() {
#ifdef MF_FLASH_BACKUP_ADDR
  mf_erase((uint32_t)(info_backup));
  mf_write((uint32_t)info_backup, info_main);
#endif
  mf_erase((uint32_t)info_main);
  mf_write((uint32_t)info_main, mf_temp);
}

void mf_load() { memcpy(mf_temp, info_main, MF_FLASH_BLOCK_SIZE); }

void mf_purge() {
  mf_erase((uint32_t)info_main);
#ifdef MF_FLASH_BACKUP_ADDR
  mf_erase((uint32_t)info_backup);
#endif
  mf_init();
}

static size_t mf_get_key_size(mf_keyinfo_t *key) {
  return sizeof(mf_keyinfo_t) + key->data_size + key->name_length;
}

static mf_keyinfo_t *mf_get_next_key(mf_keyinfo_t *key) {
  if (key->next_key == NULL) return NULL;
  return (mf_keyinfo_t *)(((uint8_t *)key) + mf_get_key_size(key));
}

static mf_keyinfo_t *mf_get_last_key(mf_flash_info_t *block) {
  if (mf_block_empty(block)) {
    return NULL;
  }

  mf_keyinfo_t *ans = &block->key;

  while (ans->next_key) {
    ans = mf_get_next_key(ans);
  }

  return ans;
}

size_t mf_len() {
  if (mf_block_empty(mf_data)) {
    return 0;
  }
  size_t len = 1;
  mf_keyinfo_t *ans = &mf_data->key;
  while ((ans = mf_get_next_key(ans)) != NULL) {
    len++;
  }
  return len;
}

mf_status_t mf_add_key(const char *name, const void *data, size_t size) {
  if (mf_search_key(name) != NULL) {
    return MF_ERR_EXIST;
  }

  size_t name_len = strlen(name) + 1;

  mf_keyinfo_t *key_buf = NULL;
  uint8_t *data_name_buf = NULL;
  mf_keyinfo_t *last_key = NULL;

  if (mf_block_empty(mf_data)) {
    key_buf = &mf_data->key;
    data_name_buf = (uint8_t *)&mf_data->key + sizeof(mf_keyinfo_t);
  } else {
    last_key = mf_get_last_key(mf_data);
    key_buf =
        (mf_keyinfo_t *)(((uint8_t *)last_key) + mf_get_key_size(last_key));
    data_name_buf = (uint8_t *)key_buf + sizeof(mf_keyinfo_t);
  }

  if ((uint8_t *)key_buf + sizeof(mf_keyinfo_t) + size + name_len >
      mf_temp + MF_FLASH_BLOCK_SIZE - 1) {
    return MF_ERR_FULL;
  }

  memcpy(data_name_buf, name, name_len);
  data_name_buf[name_len - 1] = 0;
  data_name_buf += name_len;
  memcpy(data_name_buf, data, size);

  *key_buf = (mf_keyinfo_t){
      .next_key = 0U, .name_length = name_len, .data_size = size};

  if (last_key != NULL) {
    last_key->next_key = true;
  }

  return MF_OK;
}

mf_status_t mf_del_key(const char *name) {
  if (mf_block_empty(mf_data)) {
    return MF_ERR_NULL;
  }
  mf_keyinfo_t *key = mf_search_key(name);
  if (key == NULL) {
    return MF_ERR_NULL;
  }
  mf_keyinfo_t *last_key = mf_get_last_key(mf_data);
  if (last_key == key) {
    size_t key_size = mf_get_key_size(key);
    if (&mf_data->key == key) {
      mf_data->key.name_length = 0;
      mf_data->key.data_size = 0;
      mf_data->key.next_key = false;
    } else {
      mf_keyinfo_t *prev_key = &mf_data->key;
      while (mf_get_next_key(prev_key) != key) {
        prev_key = mf_get_next_key(prev_key);
      }
      prev_key->next_key = false;
    }
    memset(key, 0, key_size);
  } else {
    size_t del_size = mf_get_key_size(key);
    size_t move_size = (uint8_t *)last_key - (uint8_t *)key +
                       mf_get_key_size(last_key) - del_size;
    uint8_t *move_src = (uint8_t *)key + del_size;
    uint8_t *move_dst = (uint8_t *)key;
    memmove(move_dst, move_src, move_size);
    memset(move_dst + move_size, 0, del_size);
  }
  return MF_OK;
}

mf_status_t mf_modify_key(const char *name, const void *data, size_t size) {
  mf_keyinfo_t *key = mf_search_key(name);
  if (key == NULL) {
    return MF_ERR_NULL;
  }
  if (key->data_size != size) {
    return MF_ERR_SIZE;
  }

  memcpy((void *)mf_get_keyinfo_ptr(key), data, size);

  return MF_OK;
}

mf_status_t mf_set_key(const char *name, const void *data, size_t size) {
  mf_keyinfo_t *key = mf_search_key(name);

  if (key == NULL) {
    return mf_add_key(name, data, size);
  }

  if (key->data_size != size) {
    mf_status_t status = mf_del_key(name);
    if (status != MF_OK) {
      return status;
    }
    return mf_add_key(name, data, size);
  }

  memcpy((void *)mf_get_keyinfo_ptr(key), data, size);

  return MF_OK;
}

mf_status_t mf_get_key(const char *name, void *data, size_t size,
                       size_t *data_size) {
  mf_keyinfo_t *key = mf_search_key(name);
  if (data_size) {
    *data_size = 0;
  }
  if (key == NULL) {
    return MF_ERR_NULL;
  }
  if (key->data_size > size) {
    return MF_ERR_SIZE;
  }

  memcpy(data, mf_get_keyinfo_ptr(key), size);

  if (data_size) {
    *data_size = key->data_size;
  }

  return MF_OK;
}

const void *mf_get_key_ptr(const char *name, size_t *data_size) {
  mf_keyinfo_t *key = mf_search_key(name);
  if (key == NULL) {
    if (data_size) *data_size = 0;
    return NULL;
  }
  if (data_size) *data_size = key->data_size;
  return mf_get_keyinfo_ptr(key);
}

const char *mf_get_keyinfo_name(mf_keyinfo_t *key) {
  return (char *)((uint8_t *)key + sizeof(mf_keyinfo_t));
}

const void *mf_get_keyinfo_ptr(mf_keyinfo_t *key) {
  return (const void *)((uint8_t *)key + sizeof(mf_keyinfo_t) +
                        key->name_length);
}

size_t mf_get_keyinfo_size(mf_keyinfo_t *key) { return key->data_size; }

mf_keyinfo_t *mf_search_key(const char *name) {
  if (mf_block_empty(mf_data)) {
    return NULL;
  }

  mf_keyinfo_t *ans = &mf_data->key;

  while (ans->next_key) {
    if (strcmp(name, mf_get_keyinfo_name(ans)) == 0) {
      return ans;
    }
    ans = mf_get_next_key(ans);
  }

  if (strcmp(name, mf_get_keyinfo_name(ans)) == 0) {
    return ans;
  } else {
    return NULL;
  }
}

void mf_foreach(bool (*fun)(mf_keyinfo_t *key, void *arg), void *arg) {
  if (mf_block_empty(mf_data)) {
    return;
  }

  mf_keyinfo_t *ans = &mf_data->key;

  while (ans->next_key) {
    if (!fun(ans, arg)) {
      return;
    }
    ans = mf_get_next_key(ans);
  }

  fun(ans, arg);

  return;
}

bool mf_iter(mf_keyinfo_t **key) {
  if (mf_block_empty(mf_data)) {
    return false;
  }
  if (*key == NULL) {
    *key = &mf_data->key;
  } else {
    *key = mf_get_next_key(*key);
  }
  if (*key == NULL) {
    return false;
  }
  return true;
}

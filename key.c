/**
 * @file key.c
 * @brief
 * 完善的按键检测库，支持长按/按住/双击/事件连发等功能，支持多按键同时检测和事件缓冲区
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-09-26
 *
 * THINK DIFFERENTLY
 */
#include "key.h"

uint16_t key_set_long_ms = 300;    // 长按时间
uint16_t key_set_hold_ms = 800;    // 按住时间
uint16_t key_set_double_ms = 200;  // 双击最大间隔时间 (0:关闭)
uint16_t key_set_continue_wait_ms = 600;  // 按住/双击按住连发等待时间 (0:关闭)
uint16_t key_set_continue_send_ms = 100;  // 按住/双击按住连发执行间隔
uint16_t key_set_continue_send_speedup = 1;  // 按住/双击按住连发执行加速
uint16_t key_set_continue_send_min_ms = 10;  // 按住/双击按住连发最小间隔

/* key callback function pointer */
static void (*key_callback)(uint16_t key_event) = NULL;
static void (*key_callback_alt)(uint8_t key, uint8_t event) = NULL;

static struct {
  uint16_t value[KEY_BUF_SIZE];
  uint8_t rd;
  uint8_t wr;
} key_buf;

#pragma pack(1)
typedef struct __key_dev {
  void (*status)(struct __key_dev *key_dev, uint8_t key_idx, uint8_t key_read);
  uint16_t count_ms;
  uint16_t count_temp;
} key_dev_t;
#pragma pack(0)

static uint8_t key_num = 0;
static key_dev_t *key_dev_p = NULL;
static uint8_t (*key_read_func)(uint8_t idx) = NULL;

static void key_update(uint16_t key_val);

static void key_status_down_check(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read);
static void key_status_down_shake(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read);
static void key_status_down_handle(key_dev_t *key_dev, uint8_t key_idx,
                                   uint8_t key_read);
static void key_status_hold_check(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read);
static void key_status_short_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                      uint8_t key_read);
static void key_status_short_up_handle(key_dev_t *key_dev, uint8_t key_idx,
                                       uint8_t key_read);
static void key_status_long_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                     uint8_t key_read);
static void key_status_long_up_handle(key_dev_t *key_dev, uint8_t key_idx,
                                      uint8_t key_read);
static void key_status_double_check(key_dev_t *key_dev, uint8_t key_idx,
                                    uint8_t key_read);
static void key_status_double_down_shake(key_dev_t *key_dev, uint8_t key_idx,
                                         uint8_t key_read);
static void key_status_double_continue_wait_check(key_dev_t *key_dev,
                                                  uint8_t key_idx,
                                                  uint8_t key_read);
static void key_status_double_continue_check(key_dev_t *key_dev,
                                             uint8_t key_idx, uint8_t key_read);
static void key_status_double_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                       uint8_t key_read);
static void key_status_double_up_handle(key_dev_t *key_dev, uint8_t key_idx,
                                        uint8_t key_read);
static void key_status_hold_handle(key_dev_t *key_dev, uint8_t key_idx,
                                   uint8_t key_read);
static void key_status_hold_continue_wait_check(key_dev_t *key_dev,
                                                uint8_t key_idx,
                                                uint8_t key_read);
static void key_status_hold_continue_check(key_dev_t *key_dev, uint8_t key_idx,
                                           uint8_t key_read);
static void key_status_hold_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                     uint8_t key_read);
static void key_status_hold_up_handle(key_dev_t *key_dev, uint8_t key_idx,
                                      uint8_t key_read);

/**
 * @brief write key vaule to buffer
 * @param  key_val - key value , (KEY_EVENT | KEY_NUMBER<<8)
 * @retval None
 */
static void key_update(uint16_t key_val) {
  if (key_callback != NULL) {
    key_callback(key_val);
    return;
  }
  if (key_callback_alt != NULL) {
    key_callback_alt(key_val >> 8, key_val & 0xff);
    return;
  }
  key_buf.value[key_buf.wr++] = key_val;
  key_buf.wr %= KEY_BUF_SIZE;

  /*
      overflow handle
  */
  if (key_buf.wr == key_buf.rd) {
    key_buf.rd++;
    key_buf.rd %= KEY_BUF_SIZE;
  }
}

uint16_t Key_Read(void) {
  if (key_buf.wr == key_buf.rd) {
    return KEY_EVENT_NULL;
  } else {
    uint16_t key_val = key_buf.value[key_buf.rd++];
    key_buf.rd %= KEY_BUF_SIZE;
    return key_val;
  }
}

static void key_status_down_check(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read) {
  if (key_read == KEY_READ_DOWN) {
    key_dev->status = key_status_down_shake;
    key_dev->count_ms = 0;
  }
}

static void key_status_down_shake(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read) {
  key_dev->count_ms += KEY_CHECK_MS;

  if (key_dev->count_ms < KEY_SHAKE_FILTER_MS) {
    return;
  }

  if (key_read == KEY_READ_DOWN) {
    key_dev->status = key_status_down_handle;
  } else {
    key_dev->status = key_status_down_check;
  }
}

static void key_status_down_handle(key_dev_t *key_dev, uint8_t key_idx,
                                   uint8_t key_read) {
  uint16_t key_val = key_idx << 8 | KEY_EVENT_DOWN;
  key_update(key_val);

  key_dev->status = key_status_hold_check;
  key_dev->count_ms = 0;
}

static void key_status_hold_check(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read) {
  key_dev->count_ms += KEY_CHECK_MS;

  if (key_dev->count_ms < key_set_long_ms) {
    if (key_read == KEY_READ_UP) {
      key_dev->count_temp = key_dev->count_ms;
      key_dev->status = key_status_short_up_shake;
    }
    return;
  }

  if (key_dev->count_ms < key_set_hold_ms) {
    if (key_read == KEY_READ_UP) {
      key_dev->count_temp = key_dev->count_ms;
      key_dev->status = key_status_long_up_shake;
    }
    return;
  }

  key_dev->status = key_status_hold_handle;
}

static void key_status_short_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                      uint8_t key_read) {
  key_dev->count_ms += KEY_CHECK_MS;

  if (key_dev->count_ms - key_dev->count_temp < KEY_SHAKE_FILTER_MS) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    key_dev->count_ms = 0;
    key_dev->status = key_status_double_check;
  } else {
    key_dev->status = key_status_hold_check;
  }
}

static void key_status_short_up_handle(key_dev_t *key_dev, uint8_t key_idx,
                                       uint8_t key_read) {
  uint16_t key_val;

  key_val = key_idx << 8 | KEY_EVENT_SHORT;

  key_update(key_val);

  key_dev->status = key_status_down_check;
}

static void key_status_long_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                     uint8_t key_read) {
  key_dev->count_ms += KEY_CHECK_MS;

  if (key_dev->count_ms - key_dev->count_temp < KEY_SHAKE_FILTER_MS) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_long_up_handle;
    key_dev->count_ms = 0;
  } else {
    key_dev->status = key_status_hold_check;
  }
}

static void key_status_long_up_handle(key_dev_t *key_dev, uint8_t key_idx,
                                      uint8_t key_read) {
  uint16_t key_val;

  key_val = key_idx << 8 | KEY_EVENT_LONG;

  key_update(key_val);

  key_dev->status = key_status_down_check;
}

static void key_status_double_check(key_dev_t *key_dev, uint8_t key_idx,
                                    uint8_t key_read) {
  key_dev->count_ms += KEY_CHECK_MS;

  if (key_dev->count_ms < key_set_double_ms) {
    if (key_read == KEY_READ_DOWN) {
      key_dev->status = key_status_double_down_shake;
      key_dev->count_temp = key_dev->count_ms;
    }
  } else {
    key_dev->status = key_status_short_up_handle;
  }
}

static void key_status_double_down_shake(key_dev_t *key_dev, uint8_t key_idx,
                                         uint8_t key_read) {
  key_dev->count_ms += KEY_CHECK_MS;

  if (key_dev->count_ms - key_dev->count_temp < KEY_SHAKE_FILTER_MS) {
    return;
  }

  if (key_read == KEY_READ_DOWN) {
    uint16_t key_val;

    key_val = key_idx << 8 | KEY_EVENT_DOUBLE;

    key_update(key_val);

    key_dev->status = key_status_double_continue_wait_check;
    key_dev->count_ms = 0;
  } else {
    key_dev->status = key_status_double_check;
  }
}

static void key_status_double_continue_wait_check(key_dev_t *key_dev,
                                                  uint8_t key_idx,
                                                  uint8_t key_read) {
  uint16_t key_val;

  key_dev->count_ms += KEY_CHECK_MS;

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_double_up_shake;
  }

  if (key_dev->count_ms < key_set_continue_wait_ms) {
    return;
  }

  if (key_set_continue_send_ms == 0) {
    return;
  }

  key_val = key_idx << 8 | KEY_EVENT_DOUBLE_CONTINUE;

  key_update(key_val);
  key_dev->status = key_status_double_continue_check;
  key_dev->count_ms = 0;
  key_dev->count_temp = key_set_continue_send_ms;
}

static void key_status_double_continue_check(key_dev_t *key_dev,
                                             uint8_t key_idx,
                                             uint8_t key_read) {
  uint16_t key_val;

  key_dev->count_ms += KEY_CHECK_MS;

  if (key_dev->count_temp < key_set_continue_send_min_ms) {
    key_dev->count_temp = key_set_continue_send_min_ms;
  }

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_double_up_shake;
    key_dev->count_temp = key_dev->count_ms;
  }

  if (key_dev->count_ms < key_dev->count_temp) {
    return;
  }

  key_val = key_idx << 8 | KEY_EVENT_DOUBLE_CONTINUE;

  key_update(key_val);
  key_dev->count_ms = 0;
  key_dev->count_temp -= key_set_continue_send_speedup;
}

static void key_status_double_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                       uint8_t key_read) {
  key_dev->count_ms += KEY_CHECK_MS;

  if (key_dev->count_ms - key_dev->count_temp < KEY_SHAKE_FILTER_MS) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_double_up_handle;
  } else {
    key_dev->status = key_status_double_continue_check;
    key_dev->count_temp = key_set_continue_send_ms;
  }
}

static void key_status_double_up_handle(key_dev_t *key_dev, uint8_t key_idx,
                                        uint8_t key_read) {
  uint16_t key_val;

  key_val = key_idx << 8 | KEY_EVENT_UP_DOUBLE;

  key_update(key_val);

  key_dev->status = key_status_down_check;
}

static void key_status_hold_handle(key_dev_t *key_dev, uint8_t key_idx,
                                   uint8_t key_read) {
  uint16_t key_val;

  key_val = key_idx << 8 | KEY_EVENT_HOLD;

  key_update(key_val);

  key_dev->status = key_status_hold_continue_wait_check;
  key_dev->count_ms = 0;
}

static void key_status_hold_continue_wait_check(key_dev_t *key_dev,
                                                uint8_t key_idx,
                                                uint8_t key_read) {
  uint16_t key_val;

  key_dev->count_ms += KEY_CHECK_MS;

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_hold_up_shake;
  }

  if (key_dev->count_ms < key_set_continue_wait_ms) {
    return;
  }

  if (key_set_continue_send_ms == 0) {
    return;
  }

  key_val = key_idx << 8 | KEY_EVENT_HOLD_CONTINUE;

  key_update(key_val);
  key_dev->status = key_status_hold_continue_check;
  key_dev->count_ms = 0;
  key_dev->count_temp = key_set_continue_send_ms;
}

static void key_status_hold_continue_check(key_dev_t *key_dev, uint8_t key_idx,
                                           uint8_t key_read) {
  uint16_t key_val;

  key_dev->count_ms += KEY_CHECK_MS;

  if (key_dev->count_temp < key_set_continue_send_min_ms) {
    key_dev->count_temp = key_set_continue_send_min_ms;
  }

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_hold_up_shake;
    key_dev->count_temp = key_dev->count_ms;
  }

  if (key_dev->count_ms < key_dev->count_temp) {
    return;
  }

  key_val = key_idx << 8 | KEY_EVENT_HOLD_CONTINUE;

  key_update(key_val);
  key_dev->count_ms = 0;
  key_dev->count_temp -= key_set_continue_send_speedup;
}

static void key_status_hold_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                     uint8_t key_read) {
  key_dev->count_ms += KEY_CHECK_MS;

  if (key_dev->count_ms - key_dev->count_temp < KEY_SHAKE_FILTER_MS) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_hold_up_handle;
  } else {
    key_dev->status = key_status_hold_continue_check;
    key_dev->count_temp = key_set_continue_send_ms;
  }
}

static void key_status_hold_up_handle(key_dev_t *key_dev, uint8_t key_idx,
                                      uint8_t key_read) {
  uint16_t key_val;

  key_val = key_idx << 8 | KEY_EVENT_UP_HOLD;

  key_update(key_val);

  key_dev->status = key_status_down_check;
}

void Key_Tick(void) {
  if (key_dev_p == NULL || key_read_func == NULL) {
    return;
  }
  for (uint8_t i = 0; i < key_num; i++) {
    key_dev_p[i].status(&key_dev_p[i], i, key_read_func(i));
  }
}

void Key_Register_Callback(void (*func)(uint16_t key_event)) {
  key_callback = func;
}

/**
 * @brief Register a callback function for key event
 * @note Once a callback function is registered, Key_Read() will not work
 * anymore
 * @note Only one callback function can be registered
 * @param  func - void func(uint8_t key, uint8_t event)
 */
void Key_Register_Callback_Alt(void (*func)(uint8_t key, uint8_t event)) {
  key_callback_alt = func;
}

void Key_Init(uint8_t (*read_func)(uint8_t idx), uint8_t num) {
  key_read_func = read_func;
  key_num = num;
  if (key_dev_p != NULL) {
    m_free(key_dev_p);
  }
  m_alloc(key_dev_p, sizeof(key_dev_t) * key_num);
  key_dev_p = (void *)key_dev_p;
  for (uint8_t i = 0; i < key_num; i++) {
    key_dev_p[i].status = key_status_down_check;
    key_dev_p[i].count_ms = 0;
  }
}

uint8_t Key_Is_Down(uint8_t key) {
  if (key_dev_p == NULL) {
    return 0;
  }
  return key_dev_p[key].status != key_status_down_check;
}

/**
 * @brief return key event name in string
 * @param  event
 * @retval char buf
 */
char *Key_Get_Event_Name(uint8_t event) {
  switch (event & 0xFF) {
    case KEY_EVENT_DOWN:
      return "DOWN";
    case KEY_EVENT_SHORT:
      return "SHORT";
    case KEY_EVENT_LONG:
      return "LONG";
    case KEY_EVENT_DOUBLE:
      return "DOUBLE";
    case KEY_EVENT_HOLD:
      return "HOLD";
    case KEY_EVENT_UP_HOLD:
      return "UP_HOLD";
    case KEY_EVENT_UP_DOUBLE:
      return "UP_DOUBLE";
    case KEY_EVENT_HOLD_CONTINUE:
      return "HOLD_CONTINUE";
    case KEY_EVENT_DOUBLE_CONTINUE:
      return "DOUBLE_CONTINUE";
    default:
      return "NULL";
  }
}

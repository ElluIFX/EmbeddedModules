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

key_setting_t key_setting = {
    .check_period_ms = 10,
    .shake_filter_ms = 20,
    .simple_event = 1,
    .complex_event = 1,
    .long_ms = 300,
    .hold_ms = 800,
    .double_ms = 0,
    .continue_wait_ms = 600,
    .continue_send_ms = 100,
    .continue_send_speedup = 1,
    .continue_send_min_ms = 10,
};

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

static void key_push_event(uint16_t key_val);
static void key_status_down_check(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read);
static void key_status_down_shake(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read);
static void key_status_hold_check(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read);
static void key_status_short_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                      uint8_t key_read);
static void key_status_long_up_shake(key_dev_t *key_dev, uint8_t key_idx,
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
static void key_status_hold_continue_wait_check(key_dev_t *key_dev,
                                                uint8_t key_idx,
                                                uint8_t key_read);
static void key_status_hold_continue_check(key_dev_t *key_dev, uint8_t key_idx,
                                           uint8_t key_read);
static void key_status_hold_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                     uint8_t key_read);

/**
 * @brief write key vaule to buffer
 * @param  key_val - key value , (KEY_EVENT | KEY_NUMBER<<8)
 * @retval None
 */
static void key_push_event(uint16_t key_val) {
  if (!key_setting.simple_event &&
      ((key_val & 0xFF) == KEY_EVENT_UP || (key_val & 0xFF) == KEY_EVENT_DOWN))
    return;
  if (!key_setting.complex_event &&
      ((key_val & 0xFF) != KEY_EVENT_UP && (key_val & 0xFF) != KEY_EVENT_DOWN))
    return;
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
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_dev->count_ms < key_setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_DOWN) {
    key_push_event(key_idx << 8 | KEY_EVENT_DOWN);

    key_dev->status = key_status_hold_check;
    key_dev->count_ms = 0;
  } else {
    key_dev->status = key_status_down_check;
  }
}

static void key_status_hold_check(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_dev->count_ms < key_setting.long_ms) {
    if (key_read == KEY_READ_UP) {
      key_dev->count_temp = key_dev->count_ms;
      key_dev->status = key_status_short_up_shake;
    }
    return;
  }

  if (key_dev->count_ms < key_setting.hold_ms) {
    if (key_read == KEY_READ_UP) {
      key_dev->count_temp = key_dev->count_ms;
      key_dev->status = key_status_long_up_shake;
    }
    return;
  }

  key_push_event(key_idx << 8 | KEY_EVENT_HOLD);
  key_dev->status = key_status_hold_continue_wait_check;
  key_dev->count_ms = 0;
}

static void key_status_short_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                      uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_dev->count_ms - key_dev->count_temp < key_setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    key_push_event(key_idx << 8 | KEY_EVENT_UP);
    key_dev->count_ms = 0;
    key_dev->status = key_status_double_check;
  } else {
    key_dev->status = key_status_hold_check;
  }
}

static void key_status_long_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                     uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_dev->count_ms - key_dev->count_temp < key_setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    key_push_event(key_idx << 8 | KEY_EVENT_UP);
    key_push_event(key_idx << 8 | KEY_EVENT_LONG);
    key_dev->status = key_status_down_check;
    key_dev->count_ms = 0;
  } else {
    key_dev->status = key_status_hold_check;
  }
}

static void key_status_double_check(key_dev_t *key_dev, uint8_t key_idx,
                                    uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_dev->count_ms < key_setting.double_ms) {
    if (key_read == KEY_READ_DOWN) {
      key_dev->status = key_status_double_down_shake;
      key_dev->count_temp = key_dev->count_ms;
    }
  } else {
    key_push_event(key_idx << 8 | KEY_EVENT_SHORT);
    key_dev->status = key_status_down_check;
  }
}

static void key_status_double_down_shake(key_dev_t *key_dev, uint8_t key_idx,
                                         uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_dev->count_ms - key_dev->count_temp < key_setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_DOWN) {
    key_push_event(key_idx << 8 | KEY_EVENT_DOWN);
    key_push_event(key_idx << 8 | KEY_EVENT_DOUBLE);

    key_dev->status = key_status_double_continue_wait_check;
    key_dev->count_ms = 0;
  } else {
    key_dev->status = key_status_double_check;
  }
}

static void key_status_double_continue_wait_check(key_dev_t *key_dev,
                                                  uint8_t key_idx,
                                                  uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_double_up_shake;
  }

  if (key_dev->count_ms < key_setting.continue_wait_ms) {
    return;
  }

  if (key_setting.continue_send_ms == 0) {
    return;
  }

  key_push_event(key_idx << 8 | KEY_EVENT_DOUBLE_CONTINUE);
  key_dev->status = key_status_double_continue_check;
  key_dev->count_ms = 0;
  key_dev->count_temp = key_setting.continue_send_ms;
}

static void key_status_double_continue_check(key_dev_t *key_dev,
                                             uint8_t key_idx,
                                             uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_dev->count_temp < key_setting.continue_send_min_ms) {
    key_dev->count_temp = key_setting.continue_send_min_ms;
  }

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_double_up_shake;
    key_dev->count_temp = key_dev->count_ms;
  }

  if (key_dev->count_ms < key_dev->count_temp) {
    return;
  }

  key_push_event(key_idx << 8 | KEY_EVENT_DOUBLE_CONTINUE);
  key_dev->count_ms = 0;
  key_dev->count_temp -= key_setting.continue_send_speedup;
}

static void key_status_double_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                       uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_dev->count_ms - key_dev->count_temp < key_setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    key_push_event(key_idx << 8 | KEY_EVENT_UP);
    key_dev->status = key_status_down_check;
  } else {
    key_dev->status = key_status_double_continue_check;
    key_dev->count_temp = key_setting.continue_send_ms;
  }
}

static void key_status_hold_continue_wait_check(key_dev_t *key_dev,
                                                uint8_t key_idx,
                                                uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_hold_up_shake;
  }

  if (key_dev->count_ms < key_setting.continue_wait_ms) {
    return;
  }

  if (key_setting.continue_send_ms == 0) {
    return;
  }

  key_push_event(key_idx << 8 | KEY_EVENT_HOLD_CONTINUE);
  key_dev->status = key_status_hold_continue_check;
  key_dev->count_ms = 0;
  key_dev->count_temp = key_setting.continue_send_ms;
}

static void key_status_hold_continue_check(key_dev_t *key_dev, uint8_t key_idx,
                                           uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_dev->count_temp < key_setting.continue_send_min_ms) {
    key_dev->count_temp = key_setting.continue_send_min_ms;
  }

  if (key_read == KEY_READ_UP) {
    key_dev->status = key_status_hold_up_shake;
    key_dev->count_temp = key_dev->count_ms;
  }

  if (key_dev->count_ms < key_dev->count_temp) {
    return;
  }

  key_push_event(key_idx << 8 | KEY_EVENT_HOLD_CONTINUE);
  key_dev->count_ms = 0;
  key_dev->count_temp -= key_setting.continue_send_speedup;
}

static void key_status_hold_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                     uint8_t key_read) {
  key_dev->count_ms += key_setting.check_period_ms;

  if (key_dev->count_ms - key_dev->count_temp < key_setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    key_push_event(key_idx << 8 | KEY_EVENT_UP);
    key_dev->status = key_status_down_check;
  } else {
    key_dev->status = key_status_hold_continue_check;
    key_dev->count_temp = key_setting.continue_send_ms;
  }
}

void Key_Tick(void) {
  if (key_dev_p == NULL || key_read_func == NULL) {
    return;
  }
  for (uint8_t i = 0; i < key_num; i++) {
    key_dev_p[i].status(&key_dev_p[i], i, key_read_func(i));
  }
}

void Key_RegisterCallback(void (*func)(uint16_t key_event)) {
  key_callback = func;
}

/**
 * @brief Register a callback function for key event
 * @note Once a callback function is registered, Key_Read() will not work
 * anymore
 * @note Only one callback function can be registered
 * @param  func - void func(uint8_t key, uint8_t event)
 */
void Key_RegisterCallbackAlt(void (*func)(uint8_t key, uint8_t event)) {
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

uint8_t Key_IsDown(uint8_t key) {
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
char *Key_GetEventName(uint8_t event) {
  switch (event & 0xFF) {
    case KEY_EVENT_DOWN:
      return "DOWN";
    case KEY_EVENT_UP:
      return "UP";
    case KEY_EVENT_SHORT:
      return "SHORT";
    case KEY_EVENT_LONG:
      return "LONG";
    case KEY_EVENT_DOUBLE:
      return "DOUBLE";
    case KEY_EVENT_HOLD:
      return "HOLD";
    case KEY_EVENT_HOLD_CONTINUE:
      return "HOLD_CON";
    case KEY_EVENT_DOUBLE_CONTINUE:
      return "DOUBLE_CON";
    default:
      return "NULL";
  }
}

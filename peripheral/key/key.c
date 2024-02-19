/**
 * @file key.c
 * @brief
 * 完善的按键检测库，支持短按/长按/按住/双击/重复等功能，支持多按键同时检测和事件缓冲区
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-09-26
 *
 * THINK DIFFERENTLY
 */
#include "key.h"

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
static void key_status_double_repeat_wait_check(key_dev_t *key_dev,
                                                uint8_t key_idx,
                                                uint8_t key_read);
static void key_status_double_repeat_check(key_dev_t *key_dev, uint8_t key_idx,
                                           uint8_t key_read);
static void key_status_double_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                       uint8_t key_read);
static void key_status_hold_repeat_wait_check(key_dev_t *key_dev,
                                              uint8_t key_idx,
                                              uint8_t key_read);
static void key_status_hold_repeat_check(key_dev_t *key_dev, uint8_t key_idx,
                                         uint8_t key_read);
static void key_status_hold_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                     uint8_t key_read);

static void key_push_event(key_dev_t *key_dev, uint16_t key_val) {
  if (!key_dev->setting.simple_event &&
      ((key_val & 0xFF) == KEY_EVENT_UP || (key_val & 0xFF) == KEY_EVENT_DOWN))
    return;
  if (!key_dev->setting.complex_event &&
      ((key_val & 0xFF) != KEY_EVENT_UP && (key_val & 0xFF) != KEY_EVENT_DOWN))
    return;
  if (key_dev->callback != NULL) {
    key_dev->callback(key_val >> 8, key_val & 0xff);
  }
  key_dev->key_buf.value[key_dev->key_buf.wr++] = key_val;
  key_dev->key_buf.wr %= KEY_BUF_SIZE;
  if (key_dev->key_buf.wr == key_dev->key_buf.rd) {  // 缓冲区溢出
    key_dev->key_buf.rd++;
    key_dev->key_buf.rd %= KEY_BUF_SIZE;
  }
}

static void key_status_down_check(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read) {
  if (key_read == KEY_READ_DOWN) {
    key_dev->key_arr[key_idx].status = key_status_down_shake;
    key_dev->key_arr[key_idx].count_ms = 0;
  }
}

static void key_status_down_shake(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_dev->key_arr[key_idx].count_ms < key_dev->setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_DOWN) {
    key_push_event(key_dev, key_idx << 8 | KEY_EVENT_DOWN);

    key_dev->key_arr[key_idx].status = key_status_hold_check;
    key_dev->key_arr[key_idx].count_ms = 0;
  } else {
    key_dev->key_arr[key_idx].status = key_status_down_check;
  }
}

static void key_status_hold_check(key_dev_t *key_dev, uint8_t key_idx,
                                  uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_dev->setting.long_ms &&
      key_dev->key_arr[key_idx].count_ms < key_dev->setting.long_ms) {
    if (key_read == KEY_READ_UP) {
      key_dev->key_arr[key_idx].count_temp = key_dev->key_arr[key_idx].count_ms;
      key_dev->key_arr[key_idx].status = key_status_short_up_shake;
    }
    return;
  }

  if (key_dev->key_arr[key_idx].count_ms < key_dev->setting.hold_ms ||
      !key_dev->setting.hold_ms) {
    if (key_read == KEY_READ_UP) {
      key_dev->key_arr[key_idx].count_temp = key_dev->key_arr[key_idx].count_ms;
      if (key_dev->setting.long_ms) {
        key_dev->key_arr[key_idx].status = key_status_long_up_shake;
      } else {
        key_dev->key_arr[key_idx].status = key_status_short_up_shake;
      }
    }
    return;
  }

  key_push_event(key_dev, key_idx << 8 | KEY_EVENT_HOLD);
  key_dev->key_arr[key_idx].status = key_status_hold_repeat_wait_check;
  key_dev->key_arr[key_idx].count_temp = 0;
  key_dev->key_arr[key_idx].count_ms = 0;
}

static void key_status_short_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                      uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_dev->key_arr[key_idx].count_ms -
          key_dev->key_arr[key_idx].count_temp <
      key_dev->setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    key_push_event(key_dev, key_idx << 8 | KEY_EVENT_UP);
    key_dev->key_arr[key_idx].count_ms = 0;
    key_dev->key_arr[key_idx].status = key_status_double_check;
  } else {
    key_dev->key_arr[key_idx].status = key_status_hold_check;
  }
}

static void key_status_long_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                     uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_dev->key_arr[key_idx].count_ms -
          key_dev->key_arr[key_idx].count_temp <
      key_dev->setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    key_push_event(key_dev, key_idx << 8 | KEY_EVENT_UP);
    key_push_event(key_dev, key_idx << 8 | KEY_EVENT_LONG);
    key_dev->key_arr[key_idx].status = key_status_down_check;
    key_dev->key_arr[key_idx].count_ms = 0;
  } else {
    key_dev->key_arr[key_idx].status = key_status_hold_check;
  }
}

static void key_status_double_check(key_dev_t *key_dev, uint8_t key_idx,
                                    uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_dev->key_arr[key_idx].count_ms < key_dev->setting.double_ms) {
    if (key_read == KEY_READ_DOWN) {
      key_dev->key_arr[key_idx].status = key_status_double_down_shake;
      key_dev->key_arr[key_idx].count_temp = key_dev->key_arr[key_idx].count_ms;
    }
  } else {
    key_push_event(key_dev, key_idx << 8 | KEY_EVENT_SHORT);
    key_dev->key_arr[key_idx].status = key_status_down_check;
  }
}

static void key_status_double_down_shake(key_dev_t *key_dev, uint8_t key_idx,
                                         uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_dev->key_arr[key_idx].count_ms -
          key_dev->key_arr[key_idx].count_temp <
      key_dev->setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_DOWN) {
    key_push_event(key_dev, key_idx << 8 | KEY_EVENT_DOWN);
    key_push_event(key_dev, key_idx << 8 | KEY_EVENT_DOUBLE);

    key_dev->key_arr[key_idx].status = key_status_double_repeat_wait_check;
    key_dev->key_arr[key_idx].count_ms = 0;
    key_dev->key_arr[key_idx].count_temp = 0;
  } else {
    key_dev->key_arr[key_idx].status = key_status_double_check;
  }
}

static void key_status_double_repeat_wait_check(key_dev_t *key_dev,
                                                uint8_t key_idx,
                                                uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_read == KEY_READ_UP) {
    key_dev->key_arr[key_idx].status = key_status_double_up_shake;
  }

  if (key_dev->key_arr[key_idx].count_ms < key_dev->setting.repeat_wait_ms) {
    return;
  }

  if (key_dev->setting.repeat_send_ms == 0) {
    return;
  }

  key_push_event(key_dev, key_idx << 8 | KEY_EVENT_DOUBLE_REPEAT);
  key_dev->key_arr[key_idx].status = key_status_double_repeat_check;
  key_dev->key_arr[key_idx].count_ms = 0;
  key_dev->key_arr[key_idx].count_temp = key_dev->setting.repeat_send_ms;
}

static void key_status_double_repeat_check(key_dev_t *key_dev, uint8_t key_idx,
                                           uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_dev->key_arr[key_idx].count_temp <
      key_dev->setting.repeat_send_min_ms) {
    key_dev->key_arr[key_idx].count_temp = key_dev->setting.repeat_send_min_ms;
  }

  if (key_read == KEY_READ_UP) {
    key_dev->key_arr[key_idx].status = key_status_double_up_shake;
    key_dev->key_arr[key_idx].count_temp = key_dev->key_arr[key_idx].count_ms;
  }

  if (key_dev->key_arr[key_idx].count_ms <
      key_dev->key_arr[key_idx].count_temp) {
    return;
  }

  key_push_event(key_dev, key_idx << 8 | KEY_EVENT_DOUBLE_REPEAT);
  key_dev->key_arr[key_idx].count_ms = 0;
  key_dev->key_arr[key_idx].count_temp -= key_dev->setting.repeat_send_speedup;
}

static void key_status_double_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                       uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_dev->key_arr[key_idx].count_ms -
          key_dev->key_arr[key_idx].count_temp <
      key_dev->setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    if (key_dev->key_arr[key_idx].count_temp) {
      key_push_event(key_dev, key_idx << 8 | KEY_EVENT_DOUBLE_REPEAT_STOP);
    }
    key_push_event(key_dev, key_idx << 8 | KEY_EVENT_UP);
    key_dev->key_arr[key_idx].status = key_status_down_check;
  } else {
    key_dev->key_arr[key_idx].status = key_status_double_repeat_check;
    key_dev->key_arr[key_idx].count_temp = key_dev->setting.repeat_send_ms;
  }
}

static void key_status_hold_repeat_wait_check(key_dev_t *key_dev,
                                              uint8_t key_idx,
                                              uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_read == KEY_READ_UP) {
    key_dev->key_arr[key_idx].status = key_status_hold_up_shake;
  }

  if (key_dev->key_arr[key_idx].count_ms < key_dev->setting.repeat_wait_ms) {
    return;
  }

  if (key_dev->setting.repeat_send_ms == 0) {
    return;
  }

  key_push_event(key_dev, key_idx << 8 | KEY_EVENT_HOLD_REPEAT);
  key_dev->key_arr[key_idx].status = key_status_hold_repeat_check;
  key_dev->key_arr[key_idx].count_ms = 0;
  key_dev->key_arr[key_idx].count_temp = key_dev->setting.repeat_send_ms;
}

static void key_status_hold_repeat_check(key_dev_t *key_dev, uint8_t key_idx,
                                         uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_dev->key_arr[key_idx].count_temp <
      key_dev->setting.repeat_send_min_ms) {
    key_dev->key_arr[key_idx].count_temp = key_dev->setting.repeat_send_min_ms;
  }

  if (key_read == KEY_READ_UP) {
    key_dev->key_arr[key_idx].status = key_status_hold_up_shake;
    key_dev->key_arr[key_idx].count_temp = key_dev->key_arr[key_idx].count_ms;
  }

  if (key_dev->key_arr[key_idx].count_ms <
      key_dev->key_arr[key_idx].count_temp) {
    return;
  }

  key_push_event(key_dev, key_idx << 8 | KEY_EVENT_HOLD_REPEAT);
  key_dev->key_arr[key_idx].count_ms = 0;
  key_dev->key_arr[key_idx].count_temp -= key_dev->setting.repeat_send_speedup;
}

static void key_status_hold_up_shake(key_dev_t *key_dev, uint8_t key_idx,
                                     uint8_t key_read) {
  key_dev->key_arr[key_idx].count_ms += key_dev->setting.check_period_ms;

  if (key_dev->key_arr[key_idx].count_ms -
          key_dev->key_arr[key_idx].count_temp <
      key_dev->setting.shake_filter_ms) {
    return;
  }

  if (key_read == KEY_READ_UP) {
    if (key_dev->key_arr[key_idx].count_temp) {
      key_push_event(key_dev, key_idx << 8 | KEY_EVENT_HOLD_REPEAT_STOP);
    }
    key_push_event(key_dev, key_idx << 8 | KEY_EVENT_UP);
    key_dev->key_arr[key_idx].status = key_status_down_check;
  } else {
    key_dev->key_arr[key_idx].status = key_status_hold_repeat_check;
    key_dev->key_arr[key_idx].count_temp = key_dev->setting.repeat_send_ms;
  }
}

key_dev_t *Key_Init(key_dev_t *key_dev, uint8_t (*read_func)(uint8_t idx),
                    uint8_t num, void (*callback)(uint8_t key, uint8_t event)) {
  const key_setting_t default_key_setting = {
      .check_period_ms = 10,
      .shake_filter_ms = 20,
      .simple_event = 1,
      .complex_event = 1,
      .long_ms = 300,
      .hold_ms = 800,
      .double_ms = 100,
      .repeat_wait_ms = 600,
      .repeat_send_ms = 100,
      .repeat_send_speedup = 4,
      .repeat_send_min_ms = 10,
  };
  if (!read_func || !num) {
    return NULL;
  }
  if (!key_dev) {
    key_dev = m_alloc(sizeof(key_dev_t) + num * sizeof(struct __key));
    if (!key_dev) return NULL;
  }
  key_dev->read_func = read_func;
  key_dev->callback = callback;
  key_dev->key_num = num;
  key_dev->setting = default_key_setting;
  key_dev->key_buf.rd = 0;
  key_dev->key_buf.wr = 0;
  for (uint8_t i = 0; i < num; i++) {
    key_dev->key_arr[i].status = key_status_down_check;
    key_dev->key_arr[i].count_ms = 0;
    key_dev->key_arr[i].count_temp = 0;
  }
  return key_dev;
}

uint16_t Key_Read(key_dev_t *key_dev) {
  if (key_dev->key_buf.wr == key_dev->key_buf.rd) {
    return KEY_EVENT_NULL;
  } else {
    uint16_t key_val = key_dev->key_buf.value[key_dev->key_buf.rd++];
    key_dev->key_buf.rd %= KEY_BUF_SIZE;
    return key_val;
  }
}

void Key_Tick(key_dev_t *key_dev) {
  for (uint8_t i = 0; i < key_dev->key_num; i++) {
    key_dev->key_arr[i].status(key_dev, i, key_dev->read_func(i));
  }
}

uint8_t Key_ReadRaw(key_dev_t *key_dev, uint8_t key) {
  return key_dev->key_arr[key].status != key_status_down_check ? KEY_READ_DOWN
                                                               : KEY_READ_UP;
}

char *Key_GetEventName(uint16_t event) {
  switch (event & 0xFF) {
    case KEY_EVENT_NULL:
      return "NULL";
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
    case KEY_EVENT_HOLD_REPEAT:
      return "HOLD_REP";
    case KEY_EVENT_DOUBLE_REPEAT:
      return "DOUBLE_REP";
    case KEY_EVENT_HOLD_REPEAT_STOP:
      return "HOLD_REP_STOP";
    case KEY_EVENT_DOUBLE_REPEAT_STOP:
      return "DOUBLE_REP_STOP";
    default:
      return "UNKNOWN";
  }
}

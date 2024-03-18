/**
 * @file ll_i2c_it.c
 * @brief LL I2C interface implementation, interrupt mode
 * @author Ellu (ellu.grif@gmail.com)
 * @version 2.0
 * @date 2023-10-15
 * @note thanks to zephyr
 * (https://elixir.bootlin.com/zephyr/v3.6.0/source/drivers/i2c/i2c_ll_stm32_v2.c)
 *
 * THINK DIFFERENTLY
 */

#include "ll_i2c.h"

#if LL_IIC_CFG_USE_IT
#if __has_include("i2c.h")
#include "i2c.h"
#include "log.h"
#if 0 /* 1: enable trace log */
#define I2C_TRA(fmt, ...) LOG_TRACE("I2C: " fmt, ##__VA_ARGS__)
#else
#define I2C_TRA(fmt, ...)
#endif

typedef struct i2c_interface {
  I2C_TypeDef* i2c;
  uint32_t len;           /* Data length */
  uint8_t* buf;           /* Data buffer */
  uint8_t stop : 1;       /* Send stop condition after transfer */
  uint8_t error : 1;      /* Error flag */
  uint8_t arblost : 1;    /* Arbitration lost flag */
  uint8_t nack : 1;       /* NACK flag */
  MOD_MUTEX_HANDLE mutex; /* Mutex handle for exclusive access */
  MOD_SEM_HANDLE semphr;  /* Semaphore handle for synchronization */
  struct i2c_interface* next;
} i2c_interface_t;

static i2c_interface_t* ifaces = NULL;

static i2c_interface_t* get_iface(I2C_TypeDef* i2c) {
  static i2c_interface_t* last_iface = NULL; /* Cache last used interface */
  if (last_iface && last_iface->i2c == i2c) {
    return last_iface;
  }
  i2c_interface_t* iface = ifaces;
  while (iface) {
    if (iface->i2c == i2c) {
      last_iface = iface;
      return iface;
    }
    iface = iface->next;
  }
  return NULL;
}

static void reset_iface(i2c_interface_t* iface) {
  iface->len = 0;
  iface->buf = NULL;
  iface->error = 0;
  iface->arblost = 0;
  iface->nack = 0;
  iface->stop = 0;
}

static void i2c_interface_init(I2C_TypeDef* i2c) {
  i2c_interface_t* iface = m_alloc(sizeof(i2c_interface_t));
  reset_iface(iface);
  iface->i2c = i2c;
  iface->mutex = MOD_MUTEX_CREATE("I2C");
  iface->semphr = MOD_SEM_CREATE("I2C", 0);
  iface->next = ifaces;
  ifaces = iface;
}

/**
 * @brief I2C传输初始化
 * @param  i2c
 * @param  msg_len  此次传输的数据长度
 * @param  addr    从机地址
 * @param  transfer 传输请求(LL_I2C_REQUEST_WRITE/LL_I2C_REQUEST_READ)
 * @param  reload   是否启用重载(有下一次连续传输)
 */
static void i2c_transfer_init(I2C_TypeDef* i2c, uint32_t msg_len, uint32_t addr,
                              uint32_t transfer, bool reload) {
  if (LL_I2C_IsEnabledReloadMode(i2c)) {
    I2C_TRA("reload %d", msg_len);
    LL_I2C_SetTransferSize(i2c, msg_len);
  } else {
    I2C_TRA("load %d", msg_len);
    LL_I2C_SetMasterAddressingMode(i2c, LL_I2C_ADDRESSING_MODE_7BIT);
    LL_I2C_SetSlaveAddr(i2c, (uint32_t)(addr));

    if (reload) {
      LL_I2C_EnableReloadMode(i2c);
    } else {
      LL_I2C_DisableReloadMode(i2c);
    }
    LL_I2C_DisableAutoEndMode(i2c);
    LL_I2C_SetTransferRequest(i2c, transfer);
    LL_I2C_SetTransferSize(i2c, msg_len);

    LL_I2C_Enable(i2c);

    LL_I2C_GenerateStartCondition(i2c);
  }
}
static void inline i2c_enable_transfer_interrupts(I2C_TypeDef* i2c) {
  LL_I2C_EnableIT_STOP(i2c);
  LL_I2C_EnableIT_NACK(i2c);
  LL_I2C_EnableIT_TC(i2c);
  LL_I2C_EnableIT_ERR(i2c);
}

static void inline i2c_disable_transfer_interrupts(I2C_TypeDef* i2c) {
  LL_I2C_DisableIT_TX(i2c);
  LL_I2C_DisableIT_RX(i2c);
  LL_I2C_DisableIT_STOP(i2c);
  LL_I2C_DisableIT_NACK(i2c);
  LL_I2C_DisableIT_TC(i2c);
}

static void i2c_master_mode_end(i2c_interface_t* iface, const bool from_isr) {
  I2C_TypeDef* i2c = iface->i2c;

  i2c_disable_transfer_interrupts(i2c);

  if (LL_I2C_IsEnabledReloadMode(i2c)) {
    LL_I2C_DisableReloadMode(i2c);
  }

  if (from_isr) {
    MOD_SEM_GIVE(iface->semphr); /* Sync with thread */
  }
}

void ll_i2c_event_irq(I2C_TypeDef* i2c) {
  i2c_interface_t* iface = get_iface(i2c);
  if (!iface) {
    return;
  }

  if (iface->len) {
    /* Send next byte */
    if (LL_I2C_IsActiveFlag_TXIS(i2c)) {
      LL_I2C_TransmitData8(i2c, *iface->buf);
      I2C_TRA("TX:%X", *iface->buf);
    }

    /* Receive next byte */
    if (LL_I2C_IsActiveFlag_RXNE(i2c)) {
      *iface->buf = LL_I2C_ReceiveData8(i2c);
      I2C_TRA("RX:%X", *iface->buf);
    }

    iface->buf++;
    iface->len--;
  }

  /* NACK received */
  if (LL_I2C_IsActiveFlag_NACK(i2c)) {
    I2C_TRA("NACK");
    LL_I2C_ClearFlag_NACK(i2c);
    iface->nack = 1U;
    /*
     * AutoEndMode is always disabled in master mode,
     * so send a stop condition manually
     */
    LL_I2C_GenerateStopCondition(i2c);
    return;
  }

  /* STOP received */
  if (LL_I2C_IsActiveFlag_STOP(i2c)) {
    I2C_TRA("STOP");
    LL_I2C_ClearFlag_STOP(i2c);
    LL_I2C_DisableReloadMode(i2c);
    goto end;
  }

  /* Transfer Complete or Transfer Complete Reload */
  if (LL_I2C_IsActiveFlag_TC(i2c) || LL_I2C_IsActiveFlag_TCR(i2c)) {
    I2C_TRA("%s", LL_I2C_IsActiveFlag_TC(i2c) ? "TC" : "TCR");
    /* Issue stop condition if necessary */
    if (iface->stop) {
      LL_I2C_GenerateStopCondition(i2c);
    } else {
      i2c_disable_transfer_interrupts(i2c);
      MOD_SEM_GIVE(iface->semphr);
    }
  }
  return;
end:
  i2c_master_mode_end(iface, true);
}

int ll_i2c_error_irq(I2C_TypeDef* i2c) {
  i2c_interface_t* iface = get_iface(i2c);
  if (!iface) {
    return -1;
  }
  // /* I2C Over-Run/Under-Run */
  // if (LL_I2C_IsActiveFlag_OVR(i2c)) {
  //   /* Clear OVR flag */
  //   I2C_TRA("OVR");
  //   LL_I2C_ClearFlag_OVR(i2c);
  // }

  /* I2C Bus error */
  if (LL_I2C_IsActiveFlag_BERR(i2c)) {
    /* Clear BERR flag */
    LL_I2C_ClearFlag_BERR(i2c);
    I2C_TRA("ERR");
    iface->error = 1;
    goto end;
  }

  /* I2C Arbitration Lost */
  if (LL_I2C_IsActiveFlag_ARLO(i2c)) {
    /* Clear ARLO flag */
    LL_I2C_ClearFlag_ARLO(i2c);
    I2C_TRA("ARLO");
    iface->arblost = 1;
    goto end;
  }

  return 0;
end:
  i2c_master_mode_end(iface, true);
  return -1;
}

void ll_i2c_combine_irq(I2C_TypeDef* i2c) {
  if (!ll_i2c_error_irq(i2c)) {
    ll_i2c_event_irq(i2c);
  }
}

/**
 * @brief 进行一次I2C传输
 * @param  i2c I2C_TypeDef
 * @param  addr 从机地址
 * @param  reload 是否启用重载(有下一次连续传输)
 * @param  stop   是否发送STOP条件
 * @param  is_tx  传输方向为发送
 * @param  buf 数据缓冲区
 * @param  len 数据长度
 * @retval  0:成功 -1:超时 -2:仲裁丢失 -3:NACK -4:错误
 */
static int i2c_transfer(I2C_TypeDef* i2c, uint8_t addr, bool reload, bool stop,
                        bool is_tx, uint8_t* buf, uint32_t len) {
  if (!buf || !len) {
    return false;
  }
  i2c_interface_t* iface = get_iface(i2c);
  if (!iface) {
    return false;
  }

  reset_iface(iface);

  // iface->is_tx = is_tx; // not used
  iface->stop = stop;
  iface->len = len;
  iface->buf = buf;

  i2c_transfer_init(i2c, len, addr,
                    is_tx ? LL_I2C_REQUEST_WRITE : LL_I2C_REQUEST_READ, reload);
  i2c_enable_transfer_interrupts(i2c);
  if (is_tx) {
    LL_I2C_EnableIT_TX(i2c); /* Enable TX interrupt */
  } else {
    LL_I2C_EnableIT_RX(i2c); /* Enable RX interrupt */
  }

  bool is_timeout = false;

  /* Block until end of transaction */
  if (!MOD_SEM_TRY_TAKE(iface->semphr, LL_I2C_CFG_SEM_TIMEOUT_MS)) {
    i2c_master_mode_end(iface, false);
    is_timeout = true;
  }

  if (is_timeout) {
    I2C_TRA("timeout");
    return -1;
  }
  if (iface->arblost) {
    I2C_TRA("arbitration lost");
    return -2;
  }
  if (iface->nack) {
    I2C_TRA("NACK");
    return -3;
  }
  if (iface->error) {
    I2C_TRA("error");
    return -4;
  }

  return 0;
}

///////////// PORTING //////////////
#define STM32_I2C_MAX_SIZE 255

void ll_i2c_internal_init(I2C_TypeDef* i2c) { i2c_interface_init(i2c); }

bool ll_i2c_internal_read(I2C_TypeDef* i2c, uint8_t addr, uint16_t reg,
                          uint8_t reg_len, uint8_t* data, uint32_t data_len) {
  uint8_t msg_len;
  i2c_interface_t* iface = get_iface(i2c);
  if (!iface) {
    return false;
  }

  if (!MOD_MUTEX_TRY_ACQUIRE(iface->mutex, LL_I2C_CFG_SEM_TIMEOUT_MS)) {
    return false;
  }

  // 对于读操作, 需要RESTART来转换读写方向, 所以不能reload

  if (reg_len == 2) {
    uint8_t addr2[2] = {reg >> 8, reg & 0xFF};
    I2C_TRA("send addr 2");
    if (0 != i2c_transfer(i2c, addr, false, false, true, addr2, 2)) {
      goto error;
    }
  } else if (reg_len == 1) {
    uint8_t addr1 = reg & 0xFF;
    I2C_TRA("send addr 1");
    if (0 != i2c_transfer(i2c, addr, false, false, true, &addr1, 1)) {
      goto error;
    }
  }

  do {
    msg_len = data_len > STM32_I2C_MAX_SIZE ? STM32_I2C_MAX_SIZE : data_len;
    I2C_TRA("read data %d", msg_len);
    if (0 != i2c_transfer(i2c, addr, (data_len - msg_len) > 0,
                          (data_len - msg_len) == 0, false, data, msg_len)) {
      goto error;
    }
    data += msg_len;
    data_len -= msg_len;
  } while (data_len);

  MOD_MUTEX_RELEASE(iface->mutex);
  return true;

error:
  MOD_MUTEX_RELEASE(iface->mutex);
  return false;
}

bool ll_i2c_internal_write(I2C_TypeDef* i2c, uint8_t addr, uint16_t reg,
                           uint8_t reg_len, uint8_t* data, uint32_t data_len) {
  uint8_t msg_len;
  i2c_interface_t* iface = get_iface(i2c);
  if (!iface) {
    return false;
  }

  if (!MOD_MUTEX_TRY_ACQUIRE(iface->mutex, LL_I2C_CFG_SEM_TIMEOUT_MS)) {
    return false;
  }

  // 对于写操作, 不需要RESTART来转换读写方向, 所以启用reload

  if (reg_len == 2) {
    uint8_t addr2[2] = {reg >> 8, reg & 0xFF};
    I2C_TRA("send addr 2");
    if (0 != i2c_transfer(i2c, addr, true, false, true, addr2, 2)) {
      goto error;
    }
  } else if (reg_len == 1) {
    uint8_t addr1 = reg & 0xFF;
    I2C_TRA("send addr 1");
    if (0 != i2c_transfer(i2c, addr, true, false, true, &addr1, 1)) {
      goto error;
    }
  }

  do {
    msg_len = data_len > STM32_I2C_MAX_SIZE ? STM32_I2C_MAX_SIZE : data_len;
    I2C_TRA("write data %d", msg_len);
    if (0 != i2c_transfer(i2c, addr, (data_len - msg_len) > 0,
                          (data_len - msg_len) == 0, true, data, msg_len)) {
      goto error;
    }
    data += msg_len;
    data_len -= msg_len;
  } while (data_len);

  MOD_MUTEX_RELEASE(iface->mutex);
  return true;

error:
  MOD_MUTEX_RELEASE(iface->mutex);
  return false;
}

bool ll_i2c_internal_transaction(I2C_TypeDef* i2c, uint8_t addr,
                                 ll_i2c_msg_t* msg, uint32_t msg_len) {
  i2c_interface_t* iface = get_iface(i2c);
  if (!iface) {
    return false;
  }

  if (!MOD_MUTEX_TRY_ACQUIRE(iface->mutex, LL_I2C_CFG_SEM_TIMEOUT_MS)) {
    return false;
  }

  uint32_t i;
  uint8_t act_len;
  uint32_t data_len;
  uint8_t* data;
  bool stop;

  for (i = 0; i < msg_len; i++) {
    data = msg[i].data;
    data_len = msg[i].len;
    // 最后一个消息必须发送STOP
    stop = (i == (msg_len - 1)) ? true : msg[i].stop;

    do {
      act_len = data_len > STM32_I2C_MAX_SIZE ? STM32_I2C_MAX_SIZE : data_len;
      data_len -= act_len;
      if (0 != i2c_transfer(i2c, addr, data_len > 0, stop, msg[i].is_tx, data,
                            act_len)) {
        goto error;
      }
      data += act_len;
    } while (data_len);
  }

  MOD_MUTEX_RELEASE(iface->mutex);
  return true;

error:
  MOD_MUTEX_RELEASE(iface->mutex);
  return false;
}

bool ll_i2c_internal_check_addr(I2C_TypeDef* i2c, uint8_t addr) {
  i2c_interface_t* iface = get_iface(i2c);
  if (!iface) {
    return false;
  }
  if (!MOD_MUTEX_TRY_ACQUIRE(iface->mutex, LL_I2C_CFG_SEM_TIMEOUT_MS)) {
    return false;
  }

  // dummy read for waking up some device
  uint8_t temp;
  ll_i2c_internal_read(i2c, addr, 0, 1, &temp, 1);
  bool ret = ll_i2c_internal_read(i2c, addr, 0, 1, &temp, 1);

  MOD_MUTEX_RELEASE(iface->mutex);
  return ret;
}

#endif /* __has_include("i2c.h") */
#else
void ll_i2c_event_irq(I2C_TypeDef* i2c) {}
int ll_i2c_error_irq(I2C_TypeDef* i2c) { return 0; }
void ll_i2c_combine_irq(I2C_TypeDef* i2c) {}

#endif /* LL_IIC_CFG_USE_IT */

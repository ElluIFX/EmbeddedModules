/**
 * @file ll_i2c.c
 * @brief LL I2C interface implementation
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2023-10-15
 *
 * THINK DIFFERENTLY
 */

#include "ll_i2c.h"

#if __has_include("i2c.h")

#include "i2c.h"
#include "log.h"

#define _INLINE __attribute__((always_inline))

#if LL_IIC_CFG_CONVERT_7BIT_ADDR
#define SLAVEADDR(addr) ((addr) << 1)
#else
#define SLAVEADDR(addr) (addr)
#endif

void ll_i2c_internal_init(I2C_TypeDef* i2c);

bool ll_i2c_internal_read(I2C_TypeDef* i2c, uint8_t addr, uint16_t reg,
                          uint8_t reg_len, uint8_t* data, uint32_t data_len);

bool ll_i2c_internal_write(I2C_TypeDef* i2c, uint8_t addr, uint16_t WriteAddr,
                           uint8_t reg_len, uint8_t* data, uint32_t data_len);

bool ll_i2c_internal_check_addr(I2C_TypeDef* i2c, uint8_t addr);
bool ll_i2c_internal_transaction(I2C_TypeDef* i2c, uint8_t addr,
                                 ll_i2c_msg_t* msg, uint32_t msg_len);

_INLINE void ll_i2c_init(I2C_TypeDef* i2c) { ll_i2c_internal_init(i2c); }
_INLINE bool ll_i2c_transaction(I2C_TypeDef* i2c, uint8_t addr,
                                ll_i2c_msg_t* msg, uint32_t msg_len) {
  return ll_i2c_internal_transaction(i2c, SLAVEADDR(addr), msg, msg_len);
}
_INLINE bool ll_i2c_write_raw(I2C_TypeDef* i2c, uint8_t addr, uint8_t* data,
                              uint32_t data_len) {
  return ll_i2c_internal_write(i2c, SLAVEADDR(addr), 0, 0, data, data_len);
}
_INLINE bool ll_i2c_read_raw(I2C_TypeDef* i2c, uint8_t addr, uint8_t* data,
                             uint32_t data_len) {
  return ll_i2c_internal_read(i2c, SLAVEADDR(addr), 0, 0, data, data_len);
}
_INLINE bool ll_i2c_read(I2C_TypeDef* i2c, uint8_t addr, uint8_t reg,
                         uint8_t* data, uint32_t data_len) {
  return ll_i2c_internal_read(i2c, SLAVEADDR(addr), reg, 1, data, data_len);
}
_INLINE bool ll_i2c_read_16addr(I2C_TypeDef* i2c, uint8_t addr, uint16_t reg,
                                uint8_t* data, uint32_t data_len) {
  return ll_i2c_internal_read(i2c, SLAVEADDR(addr), reg, 2, data, data_len);
}
_INLINE bool ll_i2c_write(I2C_TypeDef* i2c, uint8_t addr, uint8_t reg,
                          uint8_t* data, uint32_t data_len) {
  return ll_i2c_internal_write(i2c, SLAVEADDR(addr), reg, 1, data, data_len);
}
_INLINE bool ll_i2c_write_16addr(I2C_TypeDef* i2c, uint8_t addr, uint16_t reg,
                                 uint8_t* data, uint32_t data_len) {
  return ll_i2c_internal_write(i2c, SLAVEADDR(addr), reg, 2, data, data_len);
}
_INLINE bool ll_i2c_check_addr(I2C_TypeDef* i2c, uint8_t addr) {
  return ll_i2c_internal_check_addr(i2c, SLAVEADDR(addr));
}

void ll_i2c_bus_scan(I2C_TypeDef* i2c, uint8_t* addr_list, uint8_t* addr_cnt) {
  if (addr_cnt) *addr_cnt = 0;
  PRINTLN(T_FMT(T_YELLOW) "> LL I2C Bus Scan Start");
  for (uint8_t i = 1; i < 128; i++) {
    if (ll_i2c_internal_check_addr(i2c, i << 1)) {
      PRINTLN(T_FMT(T_CYAN) "- Found Device: 0x%02X (0x%02X)", i, i << 1);
      if (addr_list) *addr_list++ = i;
      if (addr_cnt) (*addr_cnt)++;
    }
  }
  PRINTLN(T_FMT(T_YELLOW) "> LL I2C Bus Scan End" T_FMT(T_RESET));
}

void ll_i2c_dump(I2C_TypeDef* i2c, uint8_t addr, uint8_t start, uint8_t stop) {
#define ITEM_PL 8
  uint8_t data[ITEM_PL] = {0};
  PRINTLN(T_FMT(T_YELLOW) "> I2C/0x%02X Reg Data", addr >> 1);
  uint8_t line = (stop - start) / ITEM_PL;
  for (uint8_t i = 0; i < line; i++) {
    ll_i2c_read(i2c, addr, start, data, ITEM_PL);
    PRINT(T_FMT(T_CYAN) "0x%02X:" T_FMT(T_RESET), start);
    for (uint8_t j = 0; j < (i == line ? (stop - start) : ITEM_PL); j++) {
      PRINT(" %02X", data[j]);
    }
    PRINTLN();
    start += ITEM_PL;
  }
  PRINTLN(T_FMT(T_YELLOW) "> Dump Reg End");
}

#endif /* __has_include("i2c.h") */

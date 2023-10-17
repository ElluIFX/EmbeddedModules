#include "sw_i2c.h"

#include "log.h"

#ifdef SW_IIC_SCL_Pin
#define I2C_READ 0x01
#define READ_CMD 1
#define WRITE_CMD 0

void SW_IIC_Init(void) {
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  // i2c_sw SCL
  GPIO_InitStruct.Pin = SW_IIC_SCL_Pin;
  HAL_GPIO_Init(SW_IIC_SCL_GPIO_Port, &GPIO_InitStruct);
  // i2c_sw SDA
  GPIO_InitStruct.Pin = SW_IIC_SDA_Pin;
  HAL_GPIO_Init(SW_IIC_SDA_GPIO_Port, &GPIO_InitStruct);
}

// 引脚置位
// #define GPIO_SetBits(GPIOx, GPIO_Pin) \
//   HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET)

// 引脚复位
// #define GPIO_ResetBits(GPIOx, GPIO_Pin) \
//   HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET)

// 读引脚状态
// #define GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) \
//   ((uint8_t)HAL_GPIO_ReadPin(GPIOx, GPIO_Pin))

// 寄存器方式:
// 引脚置位
#define GPIO_SetBits(GPIOx, GPIO_Pin) ((GPIOx)->BSRR = (uint32_t)(GPIO_Pin))

// 引脚复位
#define GPIO_ResetBits(GPIOx, GPIO_Pin) ((GPIOx)->BRR = (uint32_t)(GPIO_Pin))

// 读引脚状态
#define GPIO_ReadInputDataBit(GPIOx, GPIO_Pin) \
  ((uint8_t)(((GPIOx)->IDR & (GPIO_Pin)) != 0x00U))

// SDA引脚切换输入模式
void sda_in_mode() {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Pin = SW_IIC_SDA_Pin;
  HAL_GPIO_Init(SW_IIC_SDA_GPIO_Port, &GPIO_InitStruct);
}

// SDA引脚切换输出模式
void sda_out_mode() {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Pin = SW_IIC_SDA_Pin;
  HAL_GPIO_Init(SW_IIC_SDA_GPIO_Port, &GPIO_InitStruct);
}

// SCL引脚切换输入模式
void scl_in_mode() {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Pin = SW_IIC_SCL_Pin;
  HAL_GPIO_Init(SW_IIC_SCL_GPIO_Port, &GPIO_InitStruct);
}

// SCL引脚切换输出模式
void scl_out_mode() {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Pin = SW_IIC_SCL_Pin;
  HAL_GPIO_Init(SW_IIC_SCL_GPIO_Port, &GPIO_InitStruct);
}

#if SW_IIC_WAIT_TIME
#define wait_i2c() m_delay_us(SW_IIC_WAIT_TIME)
#define wait_i2c_long() m_delay_us(SW_IIC_WAIT_TIME_LONG)
#else
#define wait_i2c()
#define wait_i2c_long()
#endif

#define sda_high() GPIO_SetBits(SW_IIC_SDA_GPIO_Port, SW_IIC_SDA_Pin)

#define sda_low() GPIO_ResetBits(SW_IIC_SDA_GPIO_Port, SW_IIC_SDA_Pin)

#define scl_high() GPIO_SetBits(SW_IIC_SCL_GPIO_Port, SW_IIC_SCL_Pin)

#define scl_low() GPIO_ResetBits(SW_IIC_SCL_GPIO_Port, SW_IIC_SCL_Pin)

#define sda_out(out) ((out) ? sda_high() : sda_low())

#define i2c_clk_data_out() \
  scl_high();              \
  wait_i2c();              \
  scl_low()

#define i2c_port_initial() \
  sda_high();              \
  scl_high()

#define SW_IIC_ReadVal_SDA() \
  GPIO_ReadInputDataBit(SW_IIC_SDA_GPIO_Port, SW_IIC_SDA_Pin)

#define SW_IIC_ReadVal_SCL() \
  GPIO_ReadInputDataBit(SW_IIC_SCL_GPIO_Port, SW_IIC_SCL_Pin)

void i2c_start_condition() {
  sda_high();
  scl_high();
  wait_i2c();
  sda_low();
  wait_i2c();
  scl_low();
  wait_i2c_long();
}

void i2c_stop_condition() {
  sda_low();
  scl_high();
  wait_i2c();
  sda_high();
  wait_i2c();
}

uint8_t i2c_check_ack() {
  uint8_t ack;
  uint8_t i;
  uint8_t temp;
  sda_in_mode();
  scl_high();
  ack = 0;
  wait_i2c();
  for (i = 10; i > 0; i--) {
    temp = !(SW_IIC_ReadVal_SDA());
    if (temp) {
      ack = 1;
      break;
    }
  }
  scl_low();
  sda_out_mode();
  wait_i2c();
  return ack;
}

void i2c_check_not_ack() {
  sda_in_mode();
  i2c_clk_data_out();
  sda_out_mode();
  wait_i2c();
}

void i2c_slave_address(uint8_t SlaveAddr, uint8_t readwrite) {
  int x;

  if (readwrite) {
    SlaveAddr |= I2C_READ;
  } else {
    SlaveAddr &= ~I2C_READ;
  }

  scl_low();

  for (x = 7; x >= 0; x--) {
    sda_out(SlaveAddr & (1 << x));
    wait_i2c();
    i2c_clk_data_out();
  }
}

void i2c_register_address(uint8_t addr) {
  int x;

  scl_low();

  for (x = 7; x >= 0; x--) {
    sda_out(addr & (1 << x));
    wait_i2c();
    i2c_clk_data_out();
  }
}

void i2c_send_ack() {
  sda_out_mode();
  sda_low();
  wait_i2c();
  scl_high();
  wait_i2c_long();
  sda_low();
  wait_i2c_long();
  scl_low();
  sda_out_mode();
  wait_i2c();
}

void SW_IIC_Write_Data(uint8_t data) {
  int x;
  scl_low();
  for (x = 7; x >= 0; x--) {
    sda_out(data & (1 << x));
    wait_i2c();
    i2c_clk_data_out();
  }
}

uint8_t SW_IIC_Read_Data() {
  uint8_t x;
  uint8_t readdata = 0;
  sda_in_mode();
  for (x = 8; x--;) {
    scl_high();
    readdata <<= 1;
    if (SW_IIC_ReadVal_SDA()) readdata |= 0x01;
    wait_i2c();
    scl_low();
    wait_i2c();
  }
  sda_out_mode();
  return readdata;
}

uint8_t SW_IIC_Read_8addr(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t *pdata,
                          uint8_t rcnt) {
  uint8_t returnack = 1;
  uint8_t index;

  if (!rcnt) return 0;

  i2c_port_initial();
  i2c_start_condition();
  i2c_slave_address(SlaveAddr, WRITE_CMD);
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  wait_i2c();
  i2c_register_address(RegAddr);
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  wait_i2c();
  i2c_start_condition();
  i2c_slave_address(SlaveAddr, READ_CMD);
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  if (rcnt > 1) {
    for (index = 0; index < (rcnt - 1); index++) {
      wait_i2c();
      pdata[index] = SW_IIC_Read_Data();
      i2c_send_ack();
    }
  }
  wait_i2c();
  pdata[rcnt - 1] = SW_IIC_Read_Data();
  i2c_check_not_ack();
  i2c_stop_condition();

  return returnack;
}

uint8_t SW_IIC_Read_16addr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pdata,
                           uint8_t rcnt) {
  uint8_t returnack = 1;
  uint8_t index;

  if (!rcnt) return 0;

  i2c_port_initial();
  i2c_start_condition();
  // 写ID
  i2c_slave_address(SlaveAddr, WRITE_CMD);
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  wait_i2c();
  // 写高八位地址
  i2c_register_address((uint8_t)(RegAddr >> 8));
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  wait_i2c();
  // 写低八位地址
  i2c_register_address((uint8_t)RegAddr);
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  wait_i2c();
  // 重启I2C总线
  i2c_start_condition();
  // 读ID
  i2c_slave_address(SlaveAddr, READ_CMD);
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  // 循环读数据
  if (rcnt > 1) {
    for (index = 0; index < (rcnt - 1); index++) {
      wait_i2c();
      pdata[index] = SW_IIC_Read_Data();
      i2c_send_ack();
    }
  }
  wait_i2c();
  pdata[rcnt - 1] = SW_IIC_Read_Data();
  i2c_check_not_ack();
  i2c_stop_condition();

  return returnack;
}

uint8_t SW_IIC_Write_8addr(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t *pdata,
                           uint8_t rcnt) {
  uint8_t returnack = 1;
  uint8_t index;

  if (!rcnt) return 0;

  i2c_port_initial();
  i2c_start_condition();
  i2c_slave_address(SlaveAddr, WRITE_CMD);
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  wait_i2c();
  i2c_register_address(RegAddr);
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  wait_i2c();
  for (index = 0; index < rcnt; index++) {
    SW_IIC_Write_Data(pdata[index]);
    if (!i2c_check_ack()) {
      returnack = 0;
    }
    wait_i2c();
  }
  i2c_stop_condition();
  return returnack;
}

uint8_t SW_IIC_Write_16addr(uint8_t SlaveAddr, uint16_t RegAddr, uint8_t *pdata,
                            uint8_t rcnt) {
  uint8_t returnack = 1;
  uint8_t index;

  if (!rcnt) return 0;

  i2c_port_initial();
  i2c_start_condition();
  // 写ID
  i2c_slave_address(SlaveAddr, WRITE_CMD);
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  wait_i2c();
  // 写高八位地址
  i2c_register_address((uint8_t)(RegAddr >> 8));
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  wait_i2c();
  // 写低八位地址
  i2c_register_address((uint8_t)RegAddr);
  if (!i2c_check_ack()) {
    returnack = 0;
  }
  wait_i2c();
  // 写数据
  for (index = 0; index < rcnt; index++) {
    SW_IIC_Write_Data(pdata[index]);
    if (!i2c_check_ack()) {
      returnack = 0;
    }
    wait_i2c();
  }
  i2c_stop_condition();
  return returnack;
}

uint8_t SW_IIC_Check_SlaveAddr(uint8_t SlaveAddr) {
  uint8_t returnack = 1;
  i2c_start_condition();
  i2c_slave_address(SlaveAddr, WRITE_CMD);
  if (!i2c_check_ack()) {
    i2c_stop_condition();
    returnack = 0;
  }
  i2c_stop_condition();
  return returnack;
}

#endif /* SW_IIC_SCL_Pin */

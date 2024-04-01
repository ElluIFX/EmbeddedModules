
/************************************************************************************************************
**************    Include Headers
************************************************************************************************************/

#include "ee24.h"
#if EE24_ENABLE

/************************************************************************************************************
**************    Private Definitions
************************************************************************************************************/

#if (EE24_SIZE <= 2)
#define EE24_PSIZE 8
#elif (EE24_SIZE <= 16)
#define EE24_PSIZE 16
#else
#define EE24_PSIZE 32
#endif

/************************************************************************************************************
**************    Private Functions
************************************************************************************************************/

static void EE24_Lock(EE24_HandleTypeDef *Handle) {
  MOD_MUTEX_ACQUIRE(Handle->Mutex);
}

/***********************************************************************************************************/

static void EE24_UnLock(EE24_HandleTypeDef *Handle) {
  MOD_MUTEX_RELEASE(Handle->Mutex);
}

/************************************************************************************************************
**************    Public Functions
************************************************************************************************************/

#if !EE24_USE_WP_PIN
/**
 * @brief  Initialize EEPROM handle
 * @note   Initialize EEPROM handle and set EEPROM address
 *
 * @param  *Handle: Pointer to EE24_HandleTypeDef structure
 * @param  *HI2c: Pointer to I2C_HandleTypeDef structure
 * @param  I2CAddress: I2C Memory address
 *
 * @retval bool: true or false
 */
bool EE24_Init(EE24_HandleTypeDef *Handle, I2C_HandleTypeDef *HI2c,
               uint8_t I2CAddress) {
  bool answer = false;
  do {
    if ((Handle == NULL) || (HI2c == NULL)) {
      break;
    }
    Handle->HI2c = HI2c;
    Handle->Address = I2CAddress;
    if (HAL_I2C_IsDeviceReady(Handle->HI2c, Handle->Address, 2, 100) ==
        HAL_OK) {
      answer = true;
    }
  } while (0);

  return answer;
}
#else
/**
 * @brief  Initialize EEPROM handle
 * @note   Initialize EEPROM handle and set memory address
 *
 * @param  *Handle: Pointer to EE24_HandleTypeDef structure
 * @param  *HI2c: Pointer to I2C_HandleTypeDef structure
 * @param  I2CAddress: I2C Memory address
 * @param  *GPIO_TypeDef: Pointer to GPIO_TypeDef structure for Write protected
 * pin
 * @param  WpPin: PinNumber of write protected pin
 *
 * @retval bool: true or false
 */
bool EE24_Init(EE24_HandleTypeDef *Handle, I2C_HandleTypeDef *HI2c,
               uint8_t I2CAddress, GPIO_TypeDef *WpGpio, uint16_t WpPin) {
  bool answer = false;
  do {
    if ((Handle == NULL) || (HI2c == NULL) || (WpGpio == NULL)) {
      break;
    }
    Handle->HI2c = HI2c;
    Handle->Address = I2CAddress;
    Handle->WpGpio = WpGpio;
    Handle->WpPin = WpPin;
    HAL_GPIO_WritePin(Handle->WpGpio, Handle->WpPin, GPIO_PIN_SET);
    if (HAL_I2C_IsDeviceReady(Handle->HI2c, Handle->Address, 2, 100) ==
        HAL_OK) {
      answer = true;
    }
  } while (0);

  return answer;
}
#endif

/***********************************************************************************************************/

/**
 * @brief  Read from Memory
 * @note   Read data from memory and copy to an array
 *
 * @param  *Handle: Pointer to EE24_HandleTypeDef structure
 * @param  Address: Start address for read
 * @param  *Data: Pointer to copy data from EEPROM
 * @param  Len: Data length to read
 * @param  Timeout: Timeout of this operation in ms
 *
 * @retval bool: true or false
 */
bool EE24_Read(EE24_HandleTypeDef *Handle, uint32_t Address, uint8_t *Data,
               size_t Len, uint32_t Timeout) {
  EE24_Lock(Handle);
  bool answer = false;
  do {
#if ((EE24_SIZE == EE24_1KBIT) || (EE24_SIZE == EE24_2KBIT))
    if (HAL_I2C_Mem_Read(Handle->HI2c, Handle->Address, Address,
                         I2C_MEMADD_SIZE_8BIT, Data, Len, Timeout) == HAL_OK)
#elif (EE24_SIZE == EE24_4KBIT)
    if (HAL_I2C_Mem_Read(Handle->HI2c,
                         Handle->Address | ((Address & 0x0100) >> 7),
                         (Address & 0xff), I2C_MEMADD_SIZE_8BIT, Data, Len,
                         Timeout) == HAL_OK)
#elif (EE24_SIZE == EE24_8KBIT)
    if (HAL_I2C_Mem_Read(Handle->HI2c,
                         Handle->Address | ((Address & 0x0300) >> 7),
                         (Address & 0xff), I2C_MEMADD_SIZE_8BIT, Data, Len,
                         Timeout) == HAL_OK)
#elif (EE24_SIZE == EE24_16KBIT)
    if (HAL_I2C_Mem_Read(Handle->HI2c,
                         Handle->Address | ((Address & 0x0700) >> 7),
                         (Address & 0xff), I2C_MEMADD_SIZE_8BIT, Data, Len,
                         Timeout) == HAL_OK)
#else
    if (HAL_I2C_Mem_Read(Handle->HI2c, Handle->Address, Address,
                         I2C_MEMADD_SIZE_16BIT, Data, Len, Timeout) == HAL_OK)
#endif
    {
      answer = true;
    }
  } while (0);

  EE24_UnLock(Handle);
  return answer;
}

/***********************************************************************************************************/

/**
 * @brief  Write to Memory
 * @note   Write an array to memory
 *
 * @param  *Handle: Pointer to EE24_HandleTypeDef structure
 * @param  Address: Start address for write
 * @param  *Data: Pointer to copy data to EEPEOM
 * @param  Len: Data length to write
 * @param  Timeout: Timeout of this operation in ms
 *
 * @retval bool: true or false
 */
bool EE24_Write(EE24_HandleTypeDef *Handle, uint32_t Address, uint8_t *Data,
                size_t Len, uint32_t Timeout) {
  EE24_Lock(Handle);
  bool answer = false;
  do {
    uint16_t w;
    uint32_t startTime = HAL_GetTick();
#if EE24_USE_WP_PIN
    HAL_GPIO_WritePin(Handle->WpGpio, Handle->WpPin, GPIO_PIN_RESET);
#endif
    while (1) {
      w = EE24_PSIZE - (Address % EE24_PSIZE);
      if (w > Len) {
        w = Len;
      }
#if ((EE24_SIZE == EE24_1KBIT) || (EE24_SIZE == EE24_2KBIT))
      if (HAL_I2C_Mem_Write(Handle->HI2c, Handle->Address, Address,
                            I2C_MEMADD_SIZE_8BIT, Data, w, Timeout) == HAL_OK)
#elif (EE24_SIZE == EE24_4KBIT)
      if (HAL_I2C_Mem_Write(Handle->HI2c,
                            Handle->Address | ((Address & 0x0100) >> 7),
                            (Address & 0xff), I2C_MEMADD_SIZE_8BIT, Data, w,
                            Timeout) == HAL_OK)
#elif (EE24_SIZE == EE24_8KBIT)
      if (HAL_I2C_Mem_Write(Handle->HI2c,
                            Handle->Address | ((Address & 0x0300) >> 7),
                            (Address & 0xff), I2C_MEMADD_SIZE_8BIT, Data, w,
                            Timeout) == HAL_OK)
#elif (EE24_SIZE == EE24_16KBIT)
      if (HAL_I2C_Mem_Write(Handle->HI2c,
                            Handle->Address | ((Address & 0x0700) >> 7),
                            (Address & 0xff), I2C_MEMADD_SIZE_8BIT, Data, w,
                            Timeout) == HAL_OK)
#else
      if (HAL_I2C_Mem_Write(Handle->HI2c, Handle->Address, Address,
                            I2C_MEMADD_SIZE_16BIT, Data, w, Timeout) == HAL_OK)
#endif
      {
        m_delay_ms(10);
        Len -= w;
        Data += w;
        Address += w;
        if (Len == 0) {
          answer = true;
          break;
        }
        if (m_time_ms() - startTime >= Timeout) {
          break;
        }
      } else {
        break;
      }
    }
  } while (0);

#if EE24_USE_WP_PIN
  HAL_GPIO_WritePin(Handle->WpGpio, Handle->WpPin, GPIO_PIN_SET);
#endif
  EE24_UnLock(Handle);
  return answer;
}

/***********************************************************************************************************/
#endif  // EE24_ENABLE

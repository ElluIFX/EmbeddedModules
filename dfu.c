#include "dfu.h"

#include <main.h>

#define RESET_TO_BOOTLOADER_MAGIC_CODE (uint8_t)0x42
#define FLASH_SIZE (uint32_t)0x20000
#define RESET_TO_BOOTLOADER_MAGIC_CODE_ADDR \
  ((uint32_t)0x20000000 + FLASH_SIZE - (uint32_t)0x1)
#define SYS_MEM_START_ADDR ((uint32_t)0x1FFF0000)
// https://www.st.com/resource/en/application_note/cd00167594-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics.pdf

void (*SysMemBootJump)(void);
// __IO uint8_t switchToBootloader __attribute__((section(".noinit")));
#define switchToBootloader (*((uint8_t *)RESET_TO_BOOTLOADER_MAGIC_CODE_ADDR))

/**
 * @brief Add this check inside SystemInit() before any other initialization
 */
void dfu_check(void) {
  if (switchToBootloader == RESET_TO_BOOTLOADER_MAGIC_CODE) {
    switchToBootloader = 0;
    volatile uint32_t addr = SYS_MEM_START_ADDR;
    // Point the PC to the System Memory reset vector
    SysMemBootJump = (void (*)(void))(*((uint32_t *)(addr + 4)));

    HAL_RCC_DeInit();   // Reset the system clock
    SysTick->CTRL = 0;  // Reset the  SysTick Timer
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    __set_MSP(*(uint32_t *)addr);  // Set the Main Stack Pointer

    SysMemBootJump();  // Run our virtual function defined above
    while (1) {
    }
  }
}

void reset_to_dfu(void) {
  // SYSCFG->MEMRMP &= 0xFFFFFFF9;  // Remap the memory (may not be necessary)
  // SYSCFG->MEMRMP |= 1;
  switchToBootloader = RESET_TO_BOOTLOADER_MAGIC_CODE;
  NVIC_SystemReset();
}

// if ((RCC->CSR & RCC_CSR_PINRSTF) == RCC_CSR_PINRSTF) LOG_D("pin reset");
// if ((RCC->CSR & RCC_CSR_PORRSTF) == RCC_CSR_PORRSTF) LOG_D("POW reset");
// if ((RCC->CSR & RCC_CSR_SFTRSTF) == RCC_CSR_SFTRSTF) LOG_D("Soft reset");
// if ((RCC->CSR & RCC_CSR_BORRSTF) == RCC_CSR_BORRSTF) LOG_D("BOR reset");

// flash command:
// echo dfu>COM3 & choice /t 1 /d y /n >nul & STM32_Programmer_CLI -c port=usb1
// -w "${ProgramFile}" -v -g 0x08000000

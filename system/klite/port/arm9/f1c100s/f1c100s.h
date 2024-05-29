/*******************************************************************************
* @file   f1c100s
* @brief  allwinner f1c100s header
* @author kerndev@foxmail.com
*******************************************************************************/
#ifndef __F1C100S_H__
#define __F1C100S_H__

#include <stddef.h>
#include <stdint.h>

#define __IO volatile
#pragma anon_unions

typedef enum IRQn {
    IRQ_NMI = 0,
    IRQ_UART0 = 1,
    IRQ_UART1 = 2,
    IRQ_UART2 = 3,

    IRQ_OWA = 5,
    IRQ_CIR = 6,
    IRQ_TWI0 = 7,
    IRQ_TWI1 = 8,
    IRQ_TWI2 = 9,
    IRQ_SPI0 = 10,
    IRQ_SPI1 = 11,

    IRQ_TIMER0 = 13,
    IRQ_TIMER1 = 14,
    IRQ_TIMER2 = 15,
    IRQ_WATCHDOG = 16,
    IRQ_RSB = 17,
    IRQ_DMA = 18,

    IRQ_TOUCHPANEL = 20,
    IRQ_AUDIOCODEC = 21,
    IRQ_KEYADC = 22,
    IRQ_SDC0 = 23,
    IRQ_SDC1 = 24,

    IRQ_USB_OTG = 26,
    IRQ_TVD = 27,
    IRQ_TVE = 28,
    IRQ_TCON = 29,
    IRQ_DE_FE = 30,
    IRQ_DE_BE = 31,
    IRQ_CSI = 32,
    IRQ_DE_INTERLACE = 33,
    IRQ_VE = 34,
    IRQ_DAUDIO = 35,

    IRQ_PIOD = 38,
    IRQ_PIOE = 39,
    IRQ_PIOF = 40,
    IRQ_MAX = 64,
} IRQ_Type;

typedef struct {
    __IO uint32_t CFG[4];
    __IO uint32_t DATA;
    __IO uint32_t DRV[2];
    __IO uint32_t PULL[2];
} GPIO_Type;

#define GPIOA ((GPIO_Type*)(0x01C20800))
#define GPIOB ((GPIO_Type*)(0x01C20800 + 0x24))
#define GPIOC ((GPIO_Type*)(0x01C20800 + 0x24 * 2))
#define GPIOD ((GPIO_Type*)(0x01C20800 + 0x24 * 3))
#define GPIOE ((GPIO_Type*)(0x01C20800 + 0x24 * 4))

typedef struct {
    __IO uint32_t VECTOR;
    __IO uint32_t BASE_ADDR;
    __IO uint32_t RSV1;
    __IO uint32_t NMI_CTRL;
    __IO uint32_t PEND[2];
    __IO uint32_t RSV2[2];
    __IO uint32_t EN[2];
    __IO uint32_t RSV3[2];
    __IO uint32_t MASK[2];
    __IO uint32_t RSV4[2];
    __IO uint32_t RESP[2];
    __IO uint32_t RSV5[2];
    __IO uint32_t FF[2];
    __IO uint32_t RSV6[2];
    __IO uint32_t PRIO[4];
} INTC_Type;

#define INTC ((INTC_Type*)(0x01C20400))

typedef struct {
    __IO uint32_t IER;
    __IO uint32_t ISR;
    __IO uint32_t RSV1[2];
    __IO uint32_t TIM0_CTRL;
    __IO uint32_t TIM0_INTV;
    __IO uint32_t TIM0_CNT;
    __IO uint32_t RSV2;
    __IO uint32_t TIM1_CTRL;
    __IO uint32_t TIM1_INTV;
    __IO uint32_t TIM1_CNT;
    __IO uint32_t RSV3;
    __IO uint32_t TIM2_CTRL;
    __IO uint32_t TIM2_INTV;
    __IO uint32_t TIM2_CNT;
    __IO uint32_t RSV4;
    __IO uint32_t RSV5[0x40];
    __IO uint32_t AVS_CNT_CTL;
    __IO uint32_t AVS_CNT0;
    __IO uint32_t AVS_CNT1;
    __IO uint32_t AVS_CNT_DIV;
    __IO uint32_t RSV6[0x10];
    __IO uint32_t WDT_IER;
    __IO uint32_t WDT_ISR;
    __IO uint32_t RSV7[0x8];
    __IO uint32_t WDT_CTRL;
    __IO uint32_t WDT_CFG;
    __IO uint32_t WDT_MODE;
} TIMER_Type;

#define TIMER ((TIMER_Type*)(0x01C20C00))

#endif /* __F1C100S_H__ */

/*
 * SoftSPI.c
 *
 *  Created on: 2021年4月23日
 *      Author: Geralt
 */

#include "sw_spi.h"

#define SSHigh(GPIOx, Pinx) ((GPIOx)->BSRR = (Pinx))
#define SSLow(GPIOx, Pinx) ((GPIOx)->BSRR = (Pinx) << 16)
#define SSRead(GPIOx, Pinx) (((GPIOx)->IDR) & (Pinx))
#define SSDelay(n)          \
  {                         \
    volatile int16_t i = n; \
    while (i--) asm("nop"); \
  }
#define DATA_SETUP_TIME 4
#define CLK_SETUP_TIME 4

static void SW_SPI_InternalTransmit(sw_spi_t* spidev, uint8_t* data,
                                    uint32_t length) {
  while (length--) {
    for (uint8_t i = 0; i < 8; i++) {
      SSLow(spidev->sclkPort, spidev->sclkPin);
      // MSB FIRST
      if (*data & (0x80 >> i)) {
        SSHigh(spidev->mosiPort, spidev->mosiPin);
      } else {
        SSLow(spidev->mosiPort, spidev->mosiPin);
      }
      SSDelay(DATA_SETUP_TIME);
      SSHigh(spidev->sclkPort, spidev->sclkPin);
      SSDelay(CLK_SETUP_TIME);
    }
    data++;
  }
}

static void SW_SPI_InternalReceive(sw_spi_t* spidev, uint8_t* data,
                                   uint32_t length) {
  SSLow(spidev->sclkPort, spidev->sclkPin);
  SSDelay(DATA_SETUP_TIME);
  while (length--) {
    *data = 0;
    for (uint8_t i = 0; i < 8; i++) {
      // MSB FIRST
      SSHigh(spidev->sclkPort, spidev->sclkPin);
      SSDelay(DATA_SETUP_TIME);
      *data |= SSRead(spidev->misoPort, spidev->misoPin) ? (0x80 >> i) : 0;
      SSLow(spidev->sclkPort, spidev->sclkPin);
      SSDelay(DATA_SETUP_TIME);
    }
    data++;
  }
}

/**
 * @brief 初始化软件SPI
 */
void SW_SPI_Init(sw_spi_t* spidev) {
  SSHigh(spidev->csPort, spidev->csPin);
  SSHigh(spidev->mosiPort, spidev->mosiPin);
  SSHigh(spidev->sclkPort, spidev->sclkPin);
}

/**
 * @brief 传输数据
 */
void SW_SPI_Transmit(sw_spi_t* spidev, uint8_t* data, uint32_t length) {
  if (length == 0) return;

  SSLow(spidev->csPort, spidev->csPin);
  SW_SPI_InternalTransmit(spidev, data, length);
  SSHigh(spidev->csPort, spidev->csPin);
}

/**
 * @brief 接收数据
 */
void SW_SPI_Receive(sw_spi_t* spidev, uint8_t* data, uint32_t length) {
  if (length == 0) return;

  SSLow(spidev->csPort, spidev->csPin);
  SW_SPI_InternalReceive(spidev, data, length);
  SSHigh(spidev->csPort, spidev->csPin);
}

/**
 * @brief 传输的同时接收数据
 */
void SW_SPI_TransmitReceive(sw_spi_t* spidev, uint8_t* txData, uint8_t* rxData,
                            uint32_t length) {
  if (length == 0) return;
  SSLow(spidev->csPort, spidev->csPin);
  while (length--) {
    *rxData = 0;
    for (uint8_t i = 0; i < 8; i++) {
      SSLow(spidev->sclkPort, spidev->sclkPin);
      // MSB FIRST
      if ((*txData) & (0x80 >> i)) {
        SSHigh(spidev->mosiPort, spidev->mosiPin);
      } else {
        SSLow(spidev->mosiPort, spidev->mosiPin);
      }
      SSDelay(DATA_SETUP_TIME);
      SSHigh(spidev->sclkPort, spidev->sclkPin);
      SSDelay(CLK_SETUP_TIME);
      *rxData |= SSRead(spidev->misoPort, spidev->misoPin) ? (0x80 >> i) : 0;
    }
    rxData++, txData++;
  }

  SSHigh(spidev->csPort, spidev->csPin);
}

/**
 * @brief 先传输数据，再接收数据
 */
void SW_SPI_TransmitThenReceive(sw_spi_t* spidev, uint8_t* txData,
                                uint16_t nbTx, uint8_t* rxData, uint16_t nbRx) {
  SSLow(spidev->csPort, spidev->csPin);
  if (nbTx != 0) {
    SW_SPI_InternalTransmit(spidev, txData, nbTx);
  }
  if (nbRx != 0) {
    SW_SPI_InternalTransmit(spidev, rxData, nbRx);
  }
  SSHigh(spidev->csPort, spidev->csPin);
}

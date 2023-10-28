/*
 * SoftSPI.h
 *
 *  Created on: 2021年4月23日
 *      Author: Geralt
 */

#ifndef __SW_SPI_H__
#define __SW_SPI_H__

#include "modules.h"

typedef struct {
  GPIO_TypeDef* sclkPort;
  uint16_t sclkPin;
  GPIO_TypeDef* mosiPort;
  uint16_t mosiPin;
  GPIO_TypeDef* misoPort;
  uint16_t misoPin;
  GPIO_TypeDef* csPort;
  uint16_t csPin;
} sw_spi_t;

extern void SW_SPI_Init(sw_spi_t* spidev);
extern void SW_SPI_Transmit(sw_spi_t* spidev, uint8_t* data, uint32_t length);
extern void SW_SPI_Receive(sw_spi_t* spidev, uint8_t* data, uint32_t length);
extern void SW_SPI_TransmitReceive(sw_spi_t* spidev, uint8_t* txData,
                                   uint8_t* rxData, uint32_t length);
extern void SW_SPI_TransmitThenReceive(sw_spi_t* spidev, uint8_t* txData,
                                       uint16_t nbTx, uint8_t* rxData,
                                       uint16_t nbRx);

#endif /* __SW_SPI_H__ */

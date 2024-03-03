#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "main.h"

#define FLASH_BASE_ADDR (uint32_t)(FLASH_BASE)
#define FLASH_END_ADDR (uint32_t)(0x081FFFFF)

/* Base address of the Flash sectors Bank 1 */
#define ADDR_FLASH_SECTOR_0_BANK1 \
  ((uint32_t)0x08000000) /* Base @ of Sector 0, 128 Kbytes */
#define ADDR_FLASH_SECTOR_1_BANK1 \
  ((uint32_t)0x08020000) /* Base @ of Sector 1, 128 Kbytes */
#define ADDR_FLASH_SECTOR_2_BANK1 \
  ((uint32_t)0x08040000) /* Base @ of Sector 2, 128 Kbytes */
#define ADDR_FLASH_SECTOR_3_BANK1 \
  ((uint32_t)0x08060000) /* Base @ of Sector 3, 128 Kbytes */
#define ADDR_FLASH_SECTOR_4_BANK1 \
  ((uint32_t)0x08080000) /* Base @ of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5_BANK1 \
  ((uint32_t)0x080A0000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6_BANK1 \
  ((uint32_t)0x080C0000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7_BANK1 \
  ((uint32_t)0x080E0000) /* Base @ of Sector 7, 128 Kbytes */

/* Base address of the Flash sectors Bank 2 */
#define ADDR_FLASH_SECTOR_0_BANK2 \
  ((uint32_t)0x08100000) /* Base @ of Sector 0, 128 Kbytes */
#define ADDR_FLASH_SECTOR_1_BANK2 \
  ((uint32_t)0x08120000) /* Base @ of Sector 1, 128 Kbytes */
#define ADDR_FLASH_SECTOR_2_BANK2 \
  ((uint32_t)0x08140000) /* Base @ of Sector 2, 128 Kbytes */
#define ADDR_FLASH_SECTOR_3_BANK2 \
  ((uint32_t)0x08160000) /* Base @ of Sector 3, 128 Kbytes */
#define ADDR_FLASH_SECTOR_4_BANK2 \
  ((uint32_t)0x08180000) /* Base @ of Sector 4, 128 Kbytes */
#define ADDR_FLASH_SECTOR_5_BANK2 \
  ((uint32_t)0x081A0000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6_BANK2 \
  ((uint32_t)0x081C0000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7_BANK2 \
  ((uint32_t)0x081E0000) /* Base @ of Sector 7, 128 Kbytes */

/* 一块FLASH空间的大小 */
#define MF_FLASH_BLOCK_SIZE (0x1000)  // 实际是0x10000，此处为了降低内存占用

/* 主FLASH地址 */
#define MF_FLASH_MAIN_ADDR ADDR_FLASH_SECTOR_6_BANK2

/* 备份FLASH地址，注释则不使用 */
#define MF_FLASH_BACKUP_ADDR ADDR_FLASH_SECTOR_7_BANK2

/* Flash读写函数 */
static uint32_t GetSector(uint32_t Address) {
  uint32_t sector = 0;
  if (((Address < ADDR_FLASH_SECTOR_1_BANK1) &&
       (Address >= ADDR_FLASH_SECTOR_0_BANK1)) ||
      ((Address < ADDR_FLASH_SECTOR_1_BANK2) &&
       (Address >= ADDR_FLASH_SECTOR_0_BANK2))) {
    sector = FLASH_SECTOR_0;
  } else if (((Address < ADDR_FLASH_SECTOR_2_BANK1) &&
              (Address >= ADDR_FLASH_SECTOR_1_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_2_BANK2) &&
              (Address >= ADDR_FLASH_SECTOR_1_BANK2))) {
    sector = FLASH_SECTOR_1;
  } else if (((Address < ADDR_FLASH_SECTOR_3_BANK1) &&
              (Address >= ADDR_FLASH_SECTOR_2_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_3_BANK2) &&
              (Address >= ADDR_FLASH_SECTOR_2_BANK2))) {
    sector = FLASH_SECTOR_2;
  } else if (((Address < ADDR_FLASH_SECTOR_4_BANK1) &&
              (Address >= ADDR_FLASH_SECTOR_3_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_4_BANK2) &&
              (Address >= ADDR_FLASH_SECTOR_3_BANK2))) {
    sector = FLASH_SECTOR_3;
  } else if (((Address < ADDR_FLASH_SECTOR_5_BANK1) &&
              (Address >= ADDR_FLASH_SECTOR_4_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_5_BANK2) &&
              (Address >= ADDR_FLASH_SECTOR_4_BANK2))) {
    sector = FLASH_SECTOR_4;
  } else if (((Address < ADDR_FLASH_SECTOR_6_BANK1) &&
              (Address >= ADDR_FLASH_SECTOR_5_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_6_BANK2) &&
              (Address >= ADDR_FLASH_SECTOR_5_BANK2))) {
    sector = FLASH_SECTOR_5;
  } else if (((Address < ADDR_FLASH_SECTOR_7_BANK1) &&
              (Address >= ADDR_FLASH_SECTOR_6_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_7_BANK2) &&
              (Address >= ADDR_FLASH_SECTOR_6_BANK2))) {
    sector = FLASH_SECTOR_6;
  } else if (((Address < ADDR_FLASH_SECTOR_0_BANK2) &&
              (Address >= ADDR_FLASH_SECTOR_7_BANK1)) ||
             ((Address < FLASH_END_ADDR) &&
              (Address >= ADDR_FLASH_SECTOR_7_BANK2))) {
    sector = FLASH_SECTOR_7;
  } else {
    sector = FLASH_SECTOR_7;
  }

  return sector;
}

static uint32_t GetBank(uint32_t Address) {
  uint32_t bank = 0;
  if ((Address < ADDR_FLASH_SECTOR_0_BANK2) &&
      (Address >= ADDR_FLASH_SECTOR_0_BANK1)) {
    bank = FLASH_BANK_1;
  } else {
    bank = FLASH_BANK_2;
  }

  return bank;
}

__attribute__((unused)) static void mf_erase(uint32_t addr) {
  uint32_t SECTORError = 0;

  if (HAL_FLASH_Unlock() != HAL_OK) {
    return;
  }

  FLASH_EraseInitTypeDef EraseInitStruct;

  EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
  EraseInitStruct.Banks = GetBank(addr);
  EraseInitStruct.Sector = GetSector(addr);
  EraseInitStruct.NbSectors = 1;
  HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError);

  HAL_FLASH_Lock();
}

__attribute__((unused)) static void mf_write(uint32_t addr, void* buf) {
  uint32_t status = 0;
  uint32_t flash_dest = addr;
  uint32_t num_word32 = MF_FLASH_BLOCK_SIZE / 4;
  uint32_t* src = (uint32_t*)buf;

  if (HAL_FLASH_Unlock() != HAL_OK) {
    return;
  }

  for (int i = 0; i < num_word32 / 8; i++) {
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, flash_dest,
                               (uint32_t)src);
    if (status != HAL_OK) {
      break;
    }
    flash_dest += 32;
    src += 8;
  }

  HAL_FLASH_Lock();
}

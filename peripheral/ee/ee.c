
#include <string.h>

#include "ee_internal.h"

uint32_t EE_PageSize(void) { return EE_SIZE; }

uint8_t EE_PageCount(void) { return EE_PAGE_NUMBER; }

uint8_t *EE_PageAddress(uint8_t PageOffset) {
  if (PageOffset >= EE_PAGE_NUMBER) {
    return NULL;
  }
  return (uint8_t *)(EE_ADDRESS(PageOffset));
}

bool EE_Erase(uint8_t PageOffset) {
  if (PageOffset >= EE_PAGE_NUMBER) {
    return false;
  }
  bool answer = false;
  uint32_t error;
  FLASH_EraseInitTypeDef flashErase;
  do {
    HAL_FLASH_Unlock();
#if EE_ERASE == EE_ERASE_PAGE_ADDRESS
    flashErase.TypeErase = FLASH_TYPEERASE_PAGES;
    flashErase.PageAddress = EE_ADDRESS(PageOffset);
    flashErase.NbPages = 1;
#elif EE_ERASE == EE_ERASE_PAGE_NUMBER
    flashErase.TypeErase = FLASH_TYPEERASE_PAGES;
    flashErase.Page = EE_LAST_PAGE_SECTOR - PageOffset;
    flashErase.NbPages = 1;
#else
    flashErase.TypeErase = FLASH_TYPEERASE_SECTORS;
    flashErase.Sector = EE_LAST_PAGE_SECTOR - PageOffset;
    flashErase.NbSectors = 1;
#endif
#ifdef EE_BANK_SELECT
    flashErase.Banks = EE_BANK_SELECT;
#endif
#ifdef FLASH_VOLTAGE_RANGE_3
    flashErase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
#endif
#if EE_ICACHE_ENABLE
    SCB_InvalidateICache();
#endif
    if (HAL_FLASHEx_Erase(&flashErase, &error) != HAL_OK) {
      break;
    }
    if (error != 0xFFFFFFFF) {
      break;
    }
    answer = true;

  } while (0);

  HAL_FLASH_Lock();
  return answer;
}

bool EE_Read(uint8_t PageOffset, uint8_t *Data, uint32_t Size) {
  if (PageOffset >= EE_PAGE_NUMBER) {
    return false;
  }
  if (Size > EE_SIZE) {
    return false;
  }
  __IO uint8_t *Src = (__IO uint8_t *)(EE_ADDRESS(PageOffset));
  for (uint32_t i = 0; i < Size; i++) {
    *Data++ = *Src++;
  }
  return true;
}

bool EE_Write(uint8_t PageOffset, uint8_t *Data, uint32_t Size, bool Erase) {
  if (PageOffset >= EE_PAGE_NUMBER) {
    return false;
  }
  if (Size > EE_SIZE) {
    return false;
  }
  bool answer = true;
  if (Erase) {
    if (EE_Erase(PageOffset) == false) {
      return false;
    }
  }
  do {
    HAL_FLASH_Unlock();
#if EE_ICACHE_ENABLE
    SCB_InvalidateICache();
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)Data, Size);
#endif
#if (defined FLASH_TYPEPROGRAM_HALFWORD)
    for (uint32_t i = 0; i < Size; i += 2) {
      uint64_t halfWord;
      memcpy((uint8_t *)&halfWord, Data, 2);
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                            EE_ADDRESS(PageOffset) + i, halfWord) != HAL_OK) {
        answer = false;
        break;
      }
      Data += 2;
    }
#elif (defined FLASH_TYPEPROGRAM_DOUBLEWORD)
    for (uint32_t i = 0; i < Size; i += 8) {
      uint64_t doubleWord;
      memcpy((uint8_t *)&doubleWord, Data, 8);
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                            EE_ADDRESS(PageOffset) + i, doubleWord) != HAL_OK) {
        answer = false;
        break;
      }
      Data += 8;
    }
#elif (defined FLASH_TYPEPROGRAM_QUADWORD)
    for (uint32_t i = 0; i < Size; i += 16) {
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,
                            EE_ADDRESS(PageOffset) + i,
                            (uint32_t)Data) != HAL_OK) {
        answer = false;
        break;
      }
      Data += 16;
    }
#elif (defined FLASH_TYPEPROGRAM_FLASHWORD)
    for (uint32_t i = 0; i < Size; i += 32) {
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                            EE_ADDRESS(PageOffset) + i,
                            (uint32_t)Data) != HAL_OK) {
        answer = false;
        break;
      }
      Data += 32;
    }
#endif

  } while (0);

  HAL_FLASH_Lock();
#if EE_ICACHE_ENABLE
  SCB_InvalidateICache();
  SCB_CleanInvalidateDCache_by_Addr((uint32_t *)EE_ADDRESS(PageOffset), Size);
#endif
  return answer;
}

/***********************************************************************************************************/

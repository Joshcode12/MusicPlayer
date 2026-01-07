//
// Created by joshs on 1/7/2026.
//

#include "store.h"

#include "stm32g0xx.h"

typedef struct VolumeStore {
    uint16_t magic;
    uint8_t volume;
    uint8_t padding;  // Align to 32-bit
} VolumeStore_t;

bool volume_load(void) {
    const VolumeStore_t* store = (VolumeStore_t*)VOLUME_ADDR;

    if (store->magic == VOLUME_MAGIC && store->volume <= 100) {
        volume = store->volume;
        return true;
    }
    return false;
}

bool volume_save(void) {
    HAL_FLASH_Unlock();

    // Erase page
    FLASH_EraseInitTypeDef erase = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .Page = 31,  // Last page
        .NbPages = 1
    };

    uint32_t page_error;
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    // Write as 64-bit word (STM32G0 requirement)
    const uint64_t data = ((uint64_t)volume << 16) | VOLUME_MAGIC;

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                          VOLUME_ADDR, data) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
                          }

    HAL_FLASH_Lock();
    return true;
}

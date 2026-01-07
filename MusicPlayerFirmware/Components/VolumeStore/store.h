//
// Created by joshs on 1/7/2026.
//

#ifndef MUSICPLAYERFIRMWARE_STORE_H
#define MUSICPLAYERFIRMWARE_STORE_H

#include "stdbool.h"
#include "stdint.h"

#define FLASH_USER_START_ADDR   0x0800F800  // Last 2KB page
#define VOLUME_ADDR             FLASH_USER_START_ADDR
#define VOLUME_MAGIC            0xA5C3  // Validation marker

extern uint8_t volume;

bool volume_load(void);
bool volume_save(void);

#endif //MUSICPLAYERFIRMWARE_STORE_H
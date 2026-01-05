//
// Created by joshs on 1/4/2026.
//

#include "vs1053.h"

#include "main.h"
#include "spi.h"
#include "stm32g031xx.h"

static bool vs1053_write_reg(const uint8_t reg, const uint16_t value) {
    uint8_t tx_buf[4] = {
        VS1053_SCI_WRITE,
        reg,
        (uint8_t)(value >> 8),
        (uint8_t)(value & 0xFF)
    };

    VS1053_WAIT_FOR_DREQ();

    HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_RESET);
    const HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, tx_buf, sizeof(tx_buf), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_SET);

    return (status == HAL_OK);
}

static bool vs1053_read_reg(const uint8_t reg, uint16_t* value) {
    uint8_t tx_buf[4] = {
        VS1053_SCI_READ,
        reg,
        0x00,
        0x00
    };
    uint8_t rx_buf[4] = {0};

    VS1053_WAIT_FOR_DREQ();

    HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_RESET);
    const HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, sizeof(tx_buf), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_SET);

    if (status != HAL_OK)
        return false;

    *value = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];
    return true;
}

bool vs1053_init(void) {
    // Ensure CS lines idle high
    HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MP3_CS_GPIO_Port, MP3_CS_Pin, GPIO_PIN_SET);

    if (!vs1053_soft_reset())
        return false;

    vs1053_set_volume(100);

    return true;
}

bool vs1053_soft_reset(void) {
    if (!vs1053_write_reg(VS1053_REG_MODE, VS1053_MODE_SM_RESET))
        return false;

    HAL_Delay(2); // datasheet says at least 2 ms

    const uint16_t mode = VS1053_MODE_SM_SDINEW | VS1053_MODE_SM_LAYER12 | VS1053_MODE_SM_DIFF;

    if (!vs1053_write_reg(VS1053_REG_MODE, mode))
        return false;

    if (!vs1053_write_reg(VS1053_REG_MODE, mode))
        return false;

    // Set clock multiplier (same as your ESP code: 3.5x nominal)
    if (!vs1053_write_reg(VS1053_REG_CLOCKF, 0x9800))
        return false;

    // Set AUDATA (sample rate + stereo)
    if (!vs1053_write_reg(VS1053_REG_AUDATA, 0x6D60))
        return false;

    return true;
}

void vs1053_set_volume(uint8_t percent) {
    if (percent == 0) {
        vs1053_write_reg(VS1053_REG_VOLUME, 0xFEFE);
        return;
    }

    if (percent >= 100) {
        vs1053_write_reg(VS1053_REG_VOLUME, 0x0000);
        return;
    }

    // 0 = loudest, 254 = mute
    const uint8_t att = (uint8_t)((100 - percent) * 254 / 100);

    const uint16_t reg = ((uint16_t)att << 8) | att;
    vs1053_write_reg(VS1053_REG_VOLUME, reg);
}

//
// Created by joshs on 1/4/2026.
//

#include "vs1053.h"

#include "main.h"
#include "stm32g0xx_hal.h"

extern SPI_HandleTypeDef SPI_HANDLE;

static uint16_t result = 0;

static uint16_t reg_volume_value = 0;

static bool vs1053_write_reg(const uint8_t reg, const uint16_t value) {
    uint8_t tx_buf[4] = {
        VS1053_SCI_WRITE,
        reg,
        (uint8_t)(value >> 8),
        (uint8_t)(value & 0xFF)
    };

    /* Switch SPI to lower speed */
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE);
    MODIFY_REG(SPI1->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_32);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);

    VS1053_WAIT_FOR_DREQ();

    HAL_GPIO_WritePin(X_CS_GPIO_Port, X_CS_Pin, GPIO_PIN_RESET);
    const HAL_StatusTypeDef status = HAL_SPI_Transmit(&SPI_HANDLE, tx_buf, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(X_CS_GPIO_Port, X_CS_Pin, GPIO_PIN_SET);

    /* Restore SPI speed */
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE);
    MODIFY_REG(SPI1->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_8);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);

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

    /* Switch SPI to lower speed */
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE);
    MODIFY_REG(SPI1->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_32);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);

    VS1053_WAIT_FOR_DREQ();

    HAL_GPIO_WritePin(X_CS_GPIO_Port, X_CS_Pin, GPIO_PIN_RESET);
    const HAL_StatusTypeDef status =
        HAL_SPI_TransmitReceive(&SPI_HANDLE, tx_buf, rx_buf, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(X_CS_GPIO_Port, X_CS_Pin, GPIO_PIN_SET);

    /* Restore SPI speed */
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE);
    MODIFY_REG(SPI1->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_8);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);

    if (status != HAL_OK)
        return false;

    *value = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];
    return true;
}

bool vs1053_init(void) {
    // Ensure CS lines idle high
    HAL_GPIO_WritePin(X_CS_GPIO_Port, X_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_SET);

    if (!vs1053_soft_reset())
        return false;

    vs1053_set_volume(50);

    return true;
}

bool vs1053_soft_reset(void) {
    if (!vs1053_write_reg(VS1053_REG_MODE, VS1053_MODE_SM_RESET))
        return false;

    HAL_Delay(2); // datasheet says at least 2 ms

    const uint16_t mode = VS1053_MODE_SM_SDINEW | VS1053_MODE_SM_LAYER12 | VS1053_MODE_SM_DIFF;

    if (!vs1053_write_reg(VS1053_REG_MODE, mode))
        return false;

    // Set clock multiplier
    if (!vs1053_write_reg(VS1053_REG_CLOCKF, 0x9800))
        return false;

    // check if the init worked
    return vs1053_read_reg(VS1053_REG_MODE, &result) && result == mode;
}

void vs1053_set_volume(uint8_t percent) {
    if (percent > 100) percent = 100;

    static const uint8_t sci_vol_table[21] = {
        0x00,
        0x02,
        0x04,
        0x07,
        0x0A,
        0x0C,
        0x10,
        0x14,
        0x18,
        0x1C,
        0x21,
        0x27,
        0x2D,
        0x33,
        0x3A,
        0x41,
        0x49,
        0x51,
        0x5B,
        0x65,
        0xFE
    };

    const uint8_t index = 20 - (percent / 5);

    reg_volume_value = (sci_vol_table[index] << 8) | sci_vol_table[index];

    vs1053_write_reg(VS1053_REG_VOLUME, reg_volume_value);
}

void vs1053_flush_end(void) {
    const uint8_t zeros[32] = {0};

    // Set SM_CANCEL bit
    (void)vs1053_read_reg(VS1053_REG_MODE, &result);
    (void)vs1053_write_reg(VS1053_REG_MODE, result | VS1053_MODE_SM_CANCEL);

    // Send more zeros until SM_CANCEL clears
    for (int i = 0; i < 64; i++) {
        // 64 × 32 bytes = 2048 bytes max
        vs1053_send_data(zeros, 32);

        (void)vs1053_read_reg(VS1053_REG_MODE, &result);
        if (!(result & VS1053_MODE_SM_CANCEL)) {
            return; // Cancel completed
        }
    }

    // If we reach here, cancel failed
    vs1053_soft_reset();
}

void vs1053_start_new_track(void) {
    vs1053_flush_end();

    (void)vs1053_write_reg(VS1053_REG_DECODETIME, 0);
}

void vs1053_send_data(const uint8_t* data, uint16_t len) {
    /* Switch SPI to fast speed */
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE);
    MODIFY_REG(SPI1->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_8);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);

    // VS1053 SDI accepts max 32 bytes per DREQ pulse
    while (len > 0) {
        // Wait until VS1053 is ready for more data
        VS1053_WAIT_FOR_DREQ();

        const uint16_t chunk = (len > 32) ? 32 : len;

        HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_RESET);
        HAL_SPI_Transmit(&hspi1, (uint8_t*)data, chunk, HAL_MAX_DELAY);
        HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_SET);

        data += chunk;
        len -= chunk;
    }
}

uint16_t vs1053_get_current_decode_time(void) {
    if (vs1053_read_reg(VS1053_REG_DECODETIME, &result))
        return result;

    return 0;
}

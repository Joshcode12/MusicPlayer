//
// Created by joshs on 1/4/2026.
//

#include "vs1053.h"

#include "main.h"
#include "spi.h"
#include "stm32g0xx.h"

static uint16_t result = {};

static bool vs1053_write_reg(const uint8_t reg, const uint16_t value) {
    uint8_t tx_buf[4] = {
        VS1053_SCI_WRITE,
        reg,
        (uint8_t)(value >> 8),
        (uint8_t)(value & 0xFF)
    };

    VS1053_WAIT_FOR_DREQ();

    HAL_GPIO_WritePin(X_CS_GPIO_Port, X_CS_Pin, GPIO_PIN_RESET);
    const HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, tx_buf, sizeof(tx_buf), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(X_CS_GPIO_Port, X_CS_Pin, GPIO_PIN_SET);

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

    HAL_GPIO_WritePin(X_CS_GPIO_Port, X_CS_Pin, GPIO_PIN_RESET);
    const HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, sizeof(tx_buf), HAL_MAX_DELAY);
    HAL_GPIO_WritePin(X_CS_GPIO_Port, X_CS_Pin, GPIO_PIN_SET);

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

    vs1053_set_volume(70);

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
    if (!vs1053_write_reg(VS1053_REG_CLOCKF, 0x8800))
        return false;

    // Set AUDATA (sample rate + stereo)
    if (!vs1053_write_reg(VS1053_REG_AUDATA, 0x6D60))
        return false;

    // check if the init worked
    return vs1053_read_reg(VS1053_REG_MODE, &result) && result == mode;
}

void vs1053_set_volume(const uint8_t percent) {
    // Clamp input
    if (percent == 0) {
        vs1053_write_reg(VS1053_REG_VOLUME, 0xFEFE);
        return;
    }
    if (percent >= 100) {
        vs1053_write_reg(VS1053_REG_VOLUME, 0x0000);
        return;
    }

    // Logarithmic volume table (percent → VS1053 attenuation)
    static const struct {
        uint8_t percent;
        uint8_t sci_vol;
    } table[] = {
        {100, 0x00}, {95, 0x02}, {90, 0x04}, {85, 0x07},
        {80, 0x0A}, {75, 0x0C}, {70, 0x10}, {65, 0x14},
        {60, 0x18}, {55, 0x1C}, {50, 0x21}, {45, 0x27},
        {40, 0x2D}, {35, 0x33}, {30, 0x3A}, {25, 0x41},
        {20, 0x49}, {15, 0x51}, {10, 0x5B}, {5, 0x65},
        {0, 0xFE}
    };

    uint8_t sci_vol = 0xFE;

    for (uint8_t i = 0; i < (uint8_t)(sizeof(table) / sizeof(table[0])) - 1; i++) {
        const uint8_t p_hi = table[i].percent;
        const uint8_t p_lo = table[i + 1].percent;

        if (percent <= p_hi && percent >= p_lo) {
            const uint8_t v_hi = table[i].sci_vol;
            const uint8_t v_lo = table[i + 1].sci_vol;

            const uint8_t p_range = p_hi - p_lo;
            const uint8_t p_pos = percent - p_lo;

            // Linear interpolation
            sci_vol = v_lo + ((uint32_t)p_pos * (v_hi - v_lo)) / p_range;
            break;
        }
    }

    // Apply to both channels
    vs1053_write_reg(VS1053_REG_VOLUME, (sci_vol << 8) | sci_vol);
}

void vs1053_flush_end(void) {
    const uint8_t zeros[32] = {0};

    // Set SM_CANCEL bit
    (void)vs1053_read_reg(VS1053_REG_MODE, &result);
    (void)vs1053_write_reg(VS1053_REG_MODE, result | VS1053_MODE_SM_CANCEL);

    // Send more zeros until SM_CANCEL clears
    for (int i = 0; i < 64; i++) {
        // 64 × 32 bytes = 2048 bytes max
        vs1053_send_data(zeros);

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
}

void vs1053_send_data(const uint8_t *data)
{
    /* Switch SPI to fast speed */
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE);
    MODIFY_REG(SPI1->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_8);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);

    VS1053_WAIT_FOR_DREQ();

    HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, (uint8_t *)data, 32, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(XD_CS_GPIO_Port, XD_CS_Pin, GPIO_PIN_SET);

    /* Restore SPI speed */
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE);
    MODIFY_REG(SPI1->CR1, SPI_CR1_BR, SPI_BAUDRATEPRESCALER_32);
    SET_BIT(SPI1->CR1, SPI_CR1_SPE);
}


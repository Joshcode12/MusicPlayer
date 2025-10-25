/*********************************************************************
 * @file    main.c
 * @brief   Main entry point for STM32G031K8T6 firmware
 * @author  Joshua
 * @date    2025-10-24
 *
 * @note    Target MCU: STM32G031K8T6
 *********************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "gpio.h"

void delay_ms(uint32_t ms)
{
    volatile uint32_t i;
    for (i = 0; i < (ms * 1000); i++)
    {
        __asm__("nop");
    }
}

/**
 * @brief  Main program entry point
 */
int main(void)
{
    GPIO_EnablePin(GPIOC, 6, GPIO_OTYPE_PP, GPIO_MODE_OUTPUT, GPIO_SPEED_MEDIUM, GPIO_NOPULL);
    GPIO_Write(GPIOC, 6, 0); // Start OFF

    GPIO_EnablePin(GPIOB, 3, GPIO_OTYPE_PP, GPIO_MODE_INPUT, GPIO_SPEED_MEDIUM, GPIO_PULLDOWN);

    uint8_t prev_input = 0;
    uint8_t output_state = 0;

    while (1)
    {
        uint8_t current_input = GPIO_Read(GPIOB, 3);

        if (current_input == 1 && prev_input == 0)
        {
            output_state ^= 1;
            GPIO_Write(GPIOC, 6, output_state);
        }

        prev_input = current_input;
        delay_ms(20);
    }
}
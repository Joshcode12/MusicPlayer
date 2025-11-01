/*********************************************************************
 * @file    main.c
 * @brief   Main entry point for STM32G031K8T6 firmware
 * @author  Joshua
 * @date    2025-10-24
 *
 * @note    Target MCU: STM32G031K8T6
 *********************************************************************/

#include <stdint.h>

#include "clock.h"
#include "gpio.h"

static void delay_ms(uint32_t ms) {
  volatile uint32_t i;
  for (i = 0; i < (ms * 1000); i++)
    __NOP();
}

ClockPLLConfig pll_cfg = {.pll_m = 1, .pll_n = 8, .pll_p = 2, .pll_q = 4, .pll_r = 2};

/**
 * @brief  Main program entry point
 */
int main(void) {
  Clock_InitBaseClock(CLOCK_BASE_SOURCE_LSI);
  Clock_EnableMCO(MCO_SRC_LSI, CLOCK_DIV_BY_1);
  GPIO_EnablePin(GPIOB, 2, GPIO_OTYPE_PP, GPIO_MODE_OUTPUT, GPIO_SPEED_MEDIUM, GPIO_NOPULL);
  GPIO_Write(GPIOB, 2, 1);

  GPIO_EnablePin(GPIOB, 8, GPIO_OTYPE_PP, GPIO_MODE_OUTPUT, GPIO_SPEED_MEDIUM, GPIO_NOPULL);
  GPIO_Write(GPIOB, 8, 0);

  while (1) {
    delay_ms(1);
    GPIO_Toggle(GPIOB, 2);
    GPIO_Toggle(GPIOB, 8);
  }
}
/**
 * @file    clock.h
 * @brief   Clock configuration and control code for STM32G031K8T6
 * @author  Joshua
 * @date    2025-10-29
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Defines the possible system states
 */
typedef enum {
  POWER_MODE_ACTIVE,
  POWER_MODE_SLEEP,
  POWER_MODE_STOP,
  POWER_MODE_STANDBY,
} PowerMode;

/**
 * @brief Defines the possible base clock sources
 */
typedef enum {
  CLOCK_BASE_SOURCE_HSI,    // High-Speed Internal (16 MHz)
  CLOCK_BASE_SOURCE_LSE,    // Low-Speed External (32.768 kHz)
  CLOCK_BASE_SOURCE_LSI     // Low-Speed Internal (32 kHz)
} ClockBase;

/**
 * @brief Defines the HSIDIV values
 */
typedef enum {
  HSI_DIV_BY_1,      // 16 MHz
  HSI_DIV_BY_2,      // 8 MHz
  HSI_DIV_BY_4,      // 4 MHz
  HSI_DIV_BY_8,      // 2 MHz
  HSI_DIV_BY_16,     // 1 MHz
  HSI_DIV_BY_32,     // 500 kHz
  HSI_DIV_BY_64,     // 250 kHz
  HSI_DIV_BY_128,    // 125 kHz
} ClockHSIDiv;

/**
 * @brief Defines the main system clock selector
 */
typedef enum {
  SYSCLK_SRC_HSISYS  = 0b000,    ///< HSI system clock (16 MHz)
  SYSCLK_SRC_PLLRCLK = 0b010,    ///< PLL output clock (up to 64 MHz)
  SYSCLK_SRC_LSI     = 0b011,    ///< Low-speed internal clock (~32 kHz) — low-power only
  SYSCLK_SRC_LSE     = 0b100     ///< Low-speed external clock (32.768 kHz) — low-power only
} SYSCLKSource;

/**
 * @brief Defines the MCO clock selector
 */
typedef enum {
  MCO_SRC_NONE    = 0b0000,
  MCO_SRC_SYSCLK  = 0b0001,
  MCO_SRC_HSI     = 0b0011,
  MCO_SRC_PLLRCLK = 0b0101,
  MCO_SRC_LSI     = 0b0110,
  MCO_SRC_LSE     = 0b0111,
} MCOSource;

/**
 * @brief Full configuration for the PLL
 */
typedef struct {
  uint8_t pll_m;    // PLL Input Divider (1 to 8)
  uint8_t pll_n;    // PLL Multiplier (8 to 86)
  uint8_t pll_r;    // PLL R-Output Divider (2, 4, 6, 8 for SYSCLK)
  uint8_t pll_p;    // PLL P-Output Divider (2 to 32 for I2S)
  uint8_t pll_q;    // PLL Q-Output Divider (2 to 8 for TMI1, RING)
} ClockPLLConfig;

/**
 * @brief Configuration for the AHB/APB buses
 */
typedef struct {
  uint8_t ahb_div;     // 1, 2, 4, 8, 16, 64, 128, 256, 512
  uint8_t apb1_div;    // 1, 2, 4, 8, 16
  uint8_t apb2_div;    // 1, 2, 4, 8, 16
} ClockBusConfig;

/**
 * @brief Initializes the HSIDIV to SYSCLK
 * @return true if initialization succeeds, false otherwise.
 */
bool Clock_ConfigHSIDIV(ClockHSIDiv divider);

/**
 * @brief Initializes and waits for the specified base clock source to be ready.
 * @return true if initialization succeeds, false otherwise.
 */
bool Clock_InitBaseClock(ClockBase source);

/**
 * @brief De-initializes the specified base clock source.
 * @return true if deInitialization succeeds, false otherwise.
 */
bool Clock_DeinitBaseClock(ClockBase source);

/**
 * @brief Checks if a specified base clock source is stable and ready.
 * @return true if the clock is ready, false otherwise.
 */
bool Clock_IsReady(ClockBase source);

/**
 * @brief Configures the PLL with the provided parameters.
 * @return void
 */
bool Clock_ConfigurePLL(const ClockPLLConfig *config);

/**
 * @brief Configures the PLL to have PLLPCLK at 64MHZ and the parameters at the given value.
 * @return void
 */
bool Clock_ConfigurePLLPCLKTo64(ClockPLLConfig *config);

/**
 * @brief Enables the PLL and waits for the PLLRDY flag.
 * @return true if PLL is enabled successfully, false otherwise.
 */
bool Clock_EnablePLL(void);

/**
 * @brief Disables the PLL.
 * @return true if PLL is disabled successfully, false otherwise.
 */
bool Clock_DeinitPLL(void);

/**
 * @brief Configures the PreScalers for the AHB and APB buses.
 * @return void
 */
void Clock_SetBusDividers(const ClockBusConfig *config);

/**
 * @brief Switches the main SYSCLK source and handles Flash latency changes.
 * @return true on successful switch, false if the source is not ready.
 * Note:
 * - SYSCLK_SRC_HSISYS and SYSCLK_SRC_PLLRCLK are suitable for full-speed operation.
 * - SYSCLK_SRC_LSI and SYSCLK_SRC_LSE are valid but extremely slow (~32 kHz).
 *   Use them only in low-power modes or for RTC-centric applications.
 */
bool Clock_SetSystemClock(SYSCLKSource source);

/**
 * @brief Returns the frequency of the System Clock (SYSCLK).
 * @return Frequency in Hz.
 */
uint32_t Clock_GetSYSCLK(void);

/**
 * @brief Returns the frequency of the AHB Clock (HCLK).
 * @return Frequency in Hz.
 */
uint32_t Clock_GetHCLK(void);

/**
 * @brief Returns the frequency of the APB Clock (PCLK).
 * @return Frequency in Hz.
 */
uint32_t Clock_GetPCLK(void);

/**
 * @brief Enables the MCO pin to output a specified clock.
 * @return true on successful switch, false if the source is not ready.
 * Note:
 *  divider is 1-28 in steps of 2
 */
bool Clock_EnableMCO(MCOSource source, uint8_t divider);

/**
 * Disables the MCO pin to output a specified clock.
 */
void Clock_DisableMCO(void);

/**
 * @brief Configures the system and enters Stop mode.
 */
bool Clock_EnterStopMode(void);

/**
 * @brief Configures the system and enters Standby mode.
 */
bool Clock_EnterStandbyMode(void);

/**
 * @brief Configures the system and enters Sleep mode.
 */
bool Clock_EnterSleepMode(void);

/**
 * @brief Checks of the system is in a low powered mode
 * @return Returns true if the system is currently in a low-power mode.
 */
bool Clock_IsInLowPowerMode(void);
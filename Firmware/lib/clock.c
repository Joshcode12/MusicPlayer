/**
 * @file    clock.c
 * @brief   Clock configuration and control code for STM32G031K8T6
 * @author  Joshua
 * @date    2025-10-29
 */

#include "clock.h"
#include "stm32g031xx.h"

#define CLOCK_TIMEOUT 100000UL

#define HSI_FREQ 16000000UL
#define LSE_FREQ 32768UL
#define LSI_FREQ 32000UL

// --- System & VCO Constraints ---
#define SYSCLK_MAX_R 64000000UL     // PLLR output max (SYSCLK max)
#define VCO_MIN 96000000UL          // PLLN caution: VCO min
#define VCO_MAX 344000000UL         // PLLN caution: VCO max
#define VCO_INPUT_MIN 2660000UL     // PLLM caution: 2.66 MHz min
#define VCO_INPUT_MAX 16000000UL    // PLLM caution: 16 MHz max
#define PCLK_MAX 122000000UL        // PLLP caution: P output max
#define QCLK_MAX 128000000UL        // PLLQ caution: Q output max

static volatile PowerMode currentPowerMode = POWER_MODE_ACTIVE;

static uint32_t timeout = 0;

bool Clock_ConfigHSIDIV(ClockDiv divider) {
  // if SYSCLK is not from HSIDIV
  if (((RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos) != SYSCLK_SRC_HSISYS)
    return false;

  // clear and set the div
  RCC->CR &= ~RCC_CR_HSIDIV;
  RCC->CR |= (divider << RCC_CR_HSIDIV_Pos);

  // wait for ready
  while (!(RCC->CR & RCC_CR_HSIRDY))
    ;

  return true;
}

bool Clock_InitBaseClock(ClockBase source) {
  timeout = 0;

  switch (source) {
  case CLOCK_BASE_SOURCE_HSI:
    RCC->CR |= RCC_CR_HSION;

    while (!(RCC->CR & RCC_CR_HSIRDY) && (timeout++ < CLOCK_TIMEOUT))
      ;
    return (RCC->CR & RCC_CR_HSIRDY);

  case CLOCK_BASE_SOURCE_LSE:
    // Enable PWR clock and unlock backup domain
    RCC->APBENR1 |= RCC_APBENR1_PWREN;
    PWR->CR1 |= PWR_CR1_DBP;

    while (!(PWR->CR1 & PWR_CR1_DBP) && (timeout++ < CLOCK_TIMEOUT))
      ;

    if (!(PWR->CR1 & PWR_CR1_DBP))
      return false;

    // Reset backup domain (optional safety)
    RCC->BDCR |= RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;

    // Disable LSE before reconfiguring
    RCC->BDCR &= ~(RCC_BDCR_LSEON | RCC_BDCR_LSEBYP);
    // Set high drive strength
    RCC->BDCR &= ~RCC_BDCR_LSEDRV_Msk;
    RCC->BDCR |= (3UL << RCC_BDCR_LSEDRV_Pos);
    // Enable LSE
    RCC->BDCR |= RCC_BDCR_LSEON;

    timeout = 0;
    while (!(RCC->BDCR & RCC_BDCR_LSERDY) && (timeout++ < CLOCK_TIMEOUT))
      ;

    // Lock backup domain again
    PWR->CR1 &= ~PWR_CR1_DBP;
    return (RCC->BDCR & RCC_BDCR_LSERDY);

  case CLOCK_BASE_SOURCE_LSI:
    RCC->CSR |= RCC_CSR_LSION;

    while (!(RCC->CSR & RCC_CSR_LSIRDY) && (timeout++ < CLOCK_TIMEOUT))
      ;
    return (RCC->CSR & RCC_CSR_LSIRDY);

  default:
    return false;
  }
}

bool Clock_DeinitBaseClock(ClockBase source) {
  timeout = 0;

  switch (source) {
  case CLOCK_BASE_SOURCE_HSI:
    RCC->CR &= ~RCC_CR_HSION;

    while ((RCC->CR & RCC_CR_HSIRDY) && (timeout++ < CLOCK_TIMEOUT))
      ;
    return ((RCC->CR & RCC_CR_HSIRDY) == 0);

  case CLOCK_BASE_SOURCE_LSE:
    // Enable PWR and unlock backup domain
    RCC->APBENR1 |= RCC_APBENR1_PWREN;
    PWR->CR1 |= PWR_CR1_DBP;

    while (!(PWR->CR1 & PWR_CR1_DBP) && (timeout++ < CLOCK_TIMEOUT))
      ;

    if (!(PWR->CR1 & PWR_CR1_DBP))
      return false;

    // Disable RTC and CSS if using LSE
    RCC->BDCR &= ~(RCC_BDCR_RTCEN | RCC_BDCR_LSECSSON | RCC_BDCR_LSEBYP);

    // Turn off LSE
    RCC->BDCR &= ~RCC_BDCR_LSEON;

    timeout = 0;
    while ((RCC->BDCR & RCC_BDCR_LSERDY) && (timeout++ < CLOCK_TIMEOUT))
      ;

    // Lock backup domain
    PWR->CR1 &= ~PWR_CR1_DBP;
    return ((RCC->BDCR & RCC_BDCR_LSERDY) == 0);

  case CLOCK_BASE_SOURCE_LSI:
    RCC->CSR &= ~RCC_CSR_LSION;

    while ((RCC->CSR & RCC_CSR_LSIRDY) && (timeout++ < CLOCK_TIMEOUT))
      ;
    return ((RCC->CSR & RCC_CSR_LSIRDY) == 0);

  default:
    return false;
  }
}

bool Clock_IsReady(ClockBase source) {
  switch (source) {
  case CLOCK_BASE_SOURCE_HSI:
    return (RCC->CR & RCC_CR_HSIRDY);
  case CLOCK_BASE_SOURCE_LSE:
    return (RCC->BDCR & RCC_BDCR_LSERDY);
  case CLOCK_BASE_SOURCE_LSI:
    return (RCC->CSR & RCC_CSR_LSIRDY);
  default:
    return false;
  }
}

bool Clock_ConfigurePLL(const ClockPLLConfig *config) {
  if (!config)
    return false;

  // Validate PLL input and output frequency ranges
  if (config->pll_m < 1 || config->pll_m > 8)
    return false;
  if (config->pll_n < 8 || config->pll_n > 86)
    return false;
  if (config->pll_r < 2 || config->pll_r > 8)
    return false;
  if (config->pll_p < 2 || config->pll_p > 32)
    return false;
  if (config->pll_q < 2 || config->pll_q > 8)
    return false;

  // PLLM Caution Check (f_VCO_INPUT)
  uint32_t pll_vco_input = HSI_FREQ / config->pll_m;
  if (pll_vco_input < VCO_INPUT_MIN || pll_vco_input > VCO_INPUT_MAX)
    return false;

  // PLLN Caution Check (f_VCO_OUTPUT)
  uint32_t pll_vco = pll_vco_input * config->pll_n;
  if (pll_vco < VCO_MIN || pll_vco > VCO_MAX)
    return false;

  // PLLR Caution Check (f_PLLRCLK)
  uint32_t pll_sysclk = pll_vco / config->pll_r;
  if (pll_sysclk > SYSCLK_MAX_R)
    return false;

  // PLLP Caution Check (f_PLLPCLK)
  uint32_t pll_pclk = pll_vco / config->pll_p;
  if (pll_pclk > PCLK_MAX)
    return false;

  // PLLQ Caution Check (f_PLLQCLK)
  uint32_t pll_qclk = pll_vco / config->pll_q;
  if (pll_qclk > QCLK_MAX)
    return false;

  // Disable PLL
  RCC->CR &= ~RCC_CR_PLLON;
  uint32_t timeout = 0;
  while ((RCC->CR & RCC_CR_PLLRDY) && (timeout++ < CLOCK_TIMEOUT))
    ;

  // Configure PLL
  RCC->PLLCFGR = 0;

  RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSI;

  // Configure M, N, P, Q, R
  RCC->PLLCFGR |= ((config->pll_m - 1) << RCC_PLLCFGR_PLLM_Pos);
  RCC->PLLCFGR |= (config->pll_n << RCC_PLLCFGR_PLLN_Pos);
  RCC->PLLCFGR |= ((config->pll_p - 1) << RCC_PLLCFGR_PLLP_Pos);
  RCC->PLLCFGR |= ((config->pll_q - 1) << RCC_PLLCFGR_PLLQ_Pos);
  RCC->PLLCFGR |= ((config->pll_r - 1) << RCC_PLLCFGR_PLLR_Pos);

  // Enable PLL
  RCC->CR |= RCC_CR_PLLON;
  timeout = 0;
  while (!(RCC->CR & RCC_CR_PLLRDY) && (timeout++ < CLOCK_TIMEOUT))
    ;
  return (RCC->CR & RCC_CR_PLLRDY);
}

bool Clock_ConfigurePLLPCLKTo64(ClockPLLConfig *config) {
  // 16MHz f_VCO_INPUT
  config->pll_m = 1;

  // 16MHz * 8 = 128MHz f_VCO_OUTPUT
  config->pll_n = 8;

  // 128MHz / 2 = 64MHz f_PLLRCLK
  config->pll_r = 2;

  return Clock_ConfigurePLL(config);
}

bool Clock_EnablePLL(void) {
  // Enable PLLR output
  RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;

  // Enable PLL
  RCC->CR |= RCC_CR_PLLON;

  // Wait for PLL to lock
  timeout = 0;
  while (!(RCC->CR & RCC_CR_PLLRDY) && (timeout++ < CLOCK_TIMEOUT))
    ;

  return RCC->CR & RCC_CR_PLLRDY;
}

bool Clock_DeinitPLL(void) {
  // Disable PLLR output
  RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLREN;

  // Disable PLL
  RCC->CR &= ~RCC_CR_PLLON;

  // Wait for PLL to lock
  timeout = 0;
  while ((RCC->CR & RCC_CR_PLLRDY) && (timeout++ < CLOCK_TIMEOUT))
    ;

  return RCC->CR & RCC_CR_PLLRDY;
}

void Clock_SetBusDividers(const ClockBusConfig *config);

bool Clock_SetSystemClock(SYSCLKSource source) {
  switch (source) {
  case SYSCLK_SRC_HSISYS:
    if (!Clock_IsReady(CLOCK_BASE_SOURCE_HSI))
      return false;

    // Set the flash speed to zero wait states
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= (FLASH_ACR_DBG_SWEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN);

    // clear and set the new clock source
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= SYSCLK_SRC_HSISYS;

    // wait for it to be switched
    while (((RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos) != SYSCLK_SRC_HSISYS)
      ;

    return true;
  case SYSCLK_SRC_PLLRCLK:

    return true;
  case SYSCLK_SRC_LSI:
    if (!Clock_IsReady(CLOCK_BASE_SOURCE_LSI) && Clock_IsInLowPowerMode())
      return false;

    // Set the flash speed to zero wait states
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= (FLASH_ACR_DBG_SWEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN);

    // clear and set the new clock source
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= SYSCLK_SRC_LSI;

    // wait for it to be switched
    while (((RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos) != SYSCLK_SRC_LSI)
      ;

    return true;
  case SYSCLK_SRC_LSE:
    if (!Clock_IsReady(CLOCK_BASE_SOURCE_LSE) && Clock_IsInLowPowerMode())
      return false;

    // Set the flash speed to zero wait states
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= (FLASH_ACR_DBG_SWEN | FLASH_ACR_ICEN | FLASH_ACR_PRFTEN);

    // clear and set the new clock source
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= SYSCLK_SRC_LSE;

    // wait for it to be switched
    while (((RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos) != SYSCLK_SRC_LSE)
      ;

    return true;
  default:
    return false;
  }
}

uint32_t Clock_GetSYSCLK(void) {
  switch ((RCC->CFGR & RCC_CFGR_SWS) >> RCC_CFGR_SWS_Pos) {
  case SYSCLK_SRC_HSISYS:
    return HSI_FREQ / (1 << ((RCC->CR & RCC_CR_HSIDIV) >> RCC_CR_HSIDIV_Pos));
  case SYSCLK_SRC_LSE:
    return LSE_FREQ;
  case SYSCLK_SRC_LSI:
    return LSI_FREQ;
  default:
    return HSI_FREQ;
  }
}

uint32_t Clock_GetHCLK(void);

uint32_t Clock_GetPCLK(void);

bool Clock_EnableMCO(MCOSource source, ClockDiv divider) {
  if (divider < CLOCK_DIV_BY_1 || divider > CLOCK_DIV_BY_128)
    return false;

  // Enable GPIOA clock
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

  // Configure PA8 as Alternate Function mode
  GPIOA->MODER &= ~GPIO_MODER_MODE8_Msk;
  GPIOA->MODER |= (2U << GPIO_MODER_MODE8_Pos);    // Alternate function mode

  // Set AF0 for MCO function on PA8
  GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL8_Msk;
  GPIOA->AFR[1] |= (0U << GPIO_AFRH_AFSEL8_Pos);    // AF0 = MCO

  // Configure as push-pull, high speed, no pull-up/pull-down
  GPIOA->OTYPER &= ~GPIO_OTYPER_OT8;                     // Push-pull
  GPIOA->OSPEEDR |= (2U << GPIO_OSPEEDR_OSPEED8_Pos);    // High speed
  GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD8_Msk;                 // No pull-up/pull-down

  // Configure MCO source and divider
  RCC->CFGR &= ~(RCC_CFGR_MCOSEL_Msk | RCC_CFGR_MCOPRE_Msk);
  RCC->CFGR |= ((source << RCC_CFGR_MCOSEL_Pos) | (divider << RCC_CFGR_MCOPRE_Pos));

  return true;
}

void Clock_DisableMCO(void) {
  RCC->CFGR &= ~RCC_CFGR_MCOSEL_Msk;
  GPIOA->MODER &= ~GPIO_MODER_MODE8_Msk;
  GPIOA->MODER |= (0UL << GPIO_MODER_MODE8_Pos);
}

bool Clock_EnterStopMode(void);

bool Clock_EnterStandbyMode(void);

bool Clock_EnterSleepMode(void);

bool Clock_IsInLowPowerMode(void) {
  return (currentPowerMode == POWER_MODE_SLEEP || currentPowerMode == POWER_MODE_STOP ||
          currentPowerMode == POWER_MODE_STANDBY);
}
/**
 * @file    gpio.h
 * @brief   GPIO configuration and control utilities for STM32G031K8T6
 * @author  Joshua
 * @date    2025-10-24
 */

#pragma once

#include <stdint.h>

#include "stm32g031xx.h"

/**
 * @brief GPIO pin mode types
 */
typedef enum
{
    GPIO_MODE_INPUT,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_AF,
    GPIO_MODE_ANALOG
} GPIOMode;

/**
 * @brief GPIO output type
 */
typedef enum
{
    GPIO_OTYPE_PP,
    GPIO_OTYPE_OD
} GPIOOType;

/**
 * @brief GPIO output speed
 */
typedef enum
{
    GPIO_SPEED_LOW,
    GPIO_SPEED_MEDIUM,
    GPIO_SPEED_HIGH,
    GPIO_SPEED_VERY_HIGH
} GPIOSpeed;

/**
 * @brief GPIO pull resistor configuration
 *
 * Defines internal pull-up or pull-down resistors for input pins.
 */
typedef enum
{
    GPIO_NOPULL,
    GPIO_PULLUP,
    GPIO_PULLDOWN
} GPIOPull;

/**
 * @brief Enables the clock for a given GPIO port
 */
void GPIO_EnablePort(GPIO_TypeDef *port);

/**
 * @brief Configures a GPIO pin with specified settings
 */
void GPIO_EnablePin(GPIO_TypeDef *port, uint8_t pin, GPIOOType otype, GPIOMode mode, GPIOSpeed speed, GPIOPull pull);

/**
 * @brief Writes a digital value to a GPIO pin
 */
void GPIO_Write(GPIO_TypeDef *port, uint8_t pin, uint8_t value);
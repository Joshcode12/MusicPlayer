/**
 * @file    gpio.c
 * @brief   GPIO configuration and control code for STM32G031K8T6
 * @author  Joshua
 * @date    2025-10-24
 */

#include "gpio.h"

void GPIO_EnablePort(GPIO_TypeDef *port)
{
    if (port == GPIOA)
        RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    else if (port == GPIOB)
        RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
    else if (port == GPIOC)
        RCC->IOPENR |= RCC_IOPENR_GPIOCEN;
    else if (port == GPIOD)
        RCC->IOPENR |= RCC_IOPENR_GPIODEN;
    else if (port == GPIOF)
        RCC->IOPENR |= RCC_IOPENR_GPIOFEN;
}

void GPIO_EnablePin(GPIO_TypeDef *port, uint8_t pin, GPIOOType otype, GPIOMode mode, GPIOSpeed speed, GPIOPull pull)
{

    GPIO_EnablePort(port);

    // Mode
    port->MODER &= ~(0x3 << (pin << 1));
    port->MODER |= (mode << (pin << 1));

    // Output type
    port->OTYPER &= ~(1 << pin);
    port->OTYPER |= (otype << pin);

    // Speed
    port->OSPEEDR &= ~(0x3 << (pin << 1));
    port->OSPEEDR |= (speed << (pin << 1));

    // Pull-up/down
    port->PUPDR &= ~(0x3 << (pin << 1));
    port->PUPDR |= (pull << (pin << 1));
}

void GPIO_Write(GPIO_TypeDef *port, uint8_t pin, uint8_t value)
{
    port->BSRR = (1 << pin) << (value ? 0 : 16);
}

uint8_t GPIO_Read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->IDR & (1 << pin)) ? 1 : 0;
}
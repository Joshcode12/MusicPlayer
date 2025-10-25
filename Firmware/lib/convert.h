/**
 * @file    convert.h
 * @brief   Convert binary, decimal, hex values
 * @author  Joshua
 * @date    2025-10-24
 */

#pragma once

#include <stdint.h>

/**
 * @brief Converts binary
 */
uint32_t BinToDec(uint32_t bin);

/**
 * @brief Converts decimal to binary
 */
uint32_t DecToBin(uint32_t dec);
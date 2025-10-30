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
uint32_t BinToDec(uint64_t bin);

/**
 * @brief Converts decimal to binary
 */
uint64_t DecToBin(uint32_t dec);
/**
 * @file    convert.c
 * @brief   Convert binary, decimal, hex values
 * @author  Joshua
 * @date    2025-10-24
 */

#include "convert.h"

uint32_t BinToDec(uint32_t bin)
{
    uint32_t result = 0;
    uint32_t multiplier = 1;

    while (bin > 0)
    {
        if (bin % 10)
            result += multiplier;
        multiplier <<= 1;
        bin /= 10;
    }

    return result;
}

uint32_t DecToBin(uint32_t dec)
{
    uint32_t result = 0;
    uint32_t place = 1;

    while (dec > 0)
    {
        if (dec & 1)
            result += place;
        place *= 10;
        dec >>= 1;
    }

    return result;
}
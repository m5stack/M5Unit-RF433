/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file rmt_item_types.cpp
  @brief RMT releated definition and function for RF433
*/
#include "rmt_item_types.hpp"
#include <M5Utility.h>

namespace m5 {
namespace unit {
namespace rf433 {

using m5::unit::gpio::m5_rmt_item_t;

item_container_type encodeManchester(const uint8_t* data, uint32_t len, const bool MSB)
{
    item_container_type result{};
    for (uint32_t i = 0; i < len; ++i) {
        uint8_t byte = data[i];
        for (int bit = 0; bit < 8; ++bit) {
            bool b = (byte >> (MSB ? (7 - bit) : bit)) & 1;
            result.emplace_back(b ? rmt_item_one : rmt_item_zero);
        }
    }
    return result;
}

uint16_t decodeManchester(uint8_t* buf, const uint16_t buf_size, const m5::unit::gpio::m5_rmt_item_t* items,
                          const uint32_t num, const bool MSB)
{
    constexpr uint16_t DURATION_MIN = 600;
    constexpr uint16_t DURATION_MAX = 1100;
    std::vector<uint8_t> result{};
    if (!buf || !buf_size || !items || num < 2) {
        return 0;
    }

    uint16_t idx{};
    uint8_t byte{}, bit_count{};
    m5_rmt_item_t d{};
    d.level0    = 1;
    d.level1    = 0;
    d.duration0 = items[0].duration1;

    for (uint32_t i = 1; i < num && idx < buf_size - 1; ++i) {
        const auto& it       = items[i];
        d.duration1          = it.duration0;
        const uint32_t total = d.duration0 + d.duration1;

        if (total > DURATION_MIN && total < DURATION_MAX) {
            bool bit = (d.duration0 > d.duration1);  // High long -> 1
            // M5_LIB_LOGE("\t[%3u]:%u > %u ? %u", i, d.duration0, d.duration1, bit);

            if (MSB) {
                byte <<= 1;
                byte |= bit;
            } else {
                byte |= bit << bit_count;
            }

            ++bit_count;

            if (bit_count >= 8) {
                buf[idx++] = byte;
                byte       = 0;
                bit_count  = 0;
            }
        } else {
            // M5_LIB_LOGE("Invalid %s timing:[%u] d0=%u, d1=%u", manchester ? "Manchester" : "NRZ", i, d.duration0,
            //           d.duration1);
            break;
        }
        d.duration0 = it.duration1;
    }

    // Partial byte
    if (bit_count > 0) {
        if (MSB) {
            byte <<= (8 - bit_count);
        }
        buf[idx++] = byte;
    }

    return idx;
}

container_type decode2(const m5::unit::gpio::m5_rmt_item_t* items, const uint32_t num, const bool manchester,
                       const bool MSB)
{
    constexpr uint16_t DURATION_MIN = 600;
    constexpr uint16_t DURATION_MAX = 1100;
    std::vector<uint8_t> result{};
    if (!items || num < 2) {
        return result;
    }

    uint8_t byte{}, bit_count{};
    m5_rmt_item_t d{};
    d.level0    = 1;
    d.level1    = 0;
    d.duration0 = items[0].duration1;

    for (uint32_t i = 1; i < num; ++i) {
        const auto& it = items[i];
        d.duration1    = it.duration0;

        bool valid{}, bit{};
        if (manchester) {
            const uint32_t total = d.duration0 + d.duration1;
            if (total > DURATION_MIN && total < DURATION_MAX) {
                valid = true;
                bit   = (d.duration0 > d.duration1);  // High long -> 1
                // M5_LIB_LOGE("\t[%3u]:%u > %u ? %u", i, d.duration0, d.duration1, bit);
            }
        } else {
            if (d.duration0 > 500 && d.duration1 > 200) {
                valid = true;
                bit   = (d.duration0 > d.duration1);  // High long -> 1
            }
        }

        if (valid) {
            if (MSB) {
                byte <<= 1;
                byte |= bit;
            } else {
                byte |= bit << bit_count;
            }

            ++bit_count;

            if (bit_count >= 8) {
                result.push_back(byte);
                byte      = 0;
                bit_count = 0;
            }
        } else {
            // M5_LIB_LOGE("Invalid %s timing:[%u] d0=%u, d1=%u", manchester ? "Manchester" : "NRZ", i, d.duration0,
            //             d.duration1);
            break;
        }
        d.duration0 = it.duration1;
    }

    // Partial byte
    if (bit_count > 0) {
        if (MSB) {
            byte <<= (8 - bit_count);
        }
        result.push_back(byte);
    }

    return result;
}

}  // namespace rf433
}  // namespace unit
}  // namespace m5

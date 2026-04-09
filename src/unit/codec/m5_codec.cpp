/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file m5_codec.cpp
  @brief M5Unit-RF433 original protocol codec
*/
#include "m5_codec.hpp"
#include <M5Utility.hpp>

using namespace m5::unit::gpio;

namespace m5 {
namespace unit {
namespace rf433 {

namespace {

constexpr m5_rmt_item_t rmt_eof{{1000 * 5, 0, 1000 * 5, 0}};
constexpr m5_rmt_item_t preamble_array[] = {rmt_preamble, rmt_preamble, rmt_preamble, rmt_preamble, rmt_preamble,
                                            rmt_preamble, rmt_preamble, rmt_preamble, rmt_preamble, rmt_preamble};

constexpr uint16_t sof_expected_d0{2480};
constexpr uint16_t sof_expected_d1{1630};
constexpr uint16_t sof_margin{250};

inline bool is_near(const uint16_t val, const uint16_t target, const uint16_t margin)
{
    return (val >= (target - margin)) && (val <= (target + margin));
}

}  // namespace

item_container_type M5Codec::encode(const uint8_t* payload, uint32_t payload_len)
{
    item_container_type buf;

    // Payload (Manchester encoded)
    auto payload_items = encodeManchester(payload, payload_len);

    // Length (1 byte)
    uint8_t ps     = static_cast<uint8_t>(payload_len);
    auto len_items = encodeManchester(&ps, 1);

    // Send count (1 byte)
    auto count_items = encodeManchester(&_send_count, 1);
    ++_send_count;

    // Identifier (1 byte)
    auto id_items = encodeManchester(&_comm_id, 1);

    // CRC8 (1 byte, payload only)
    m5::utility::CRC8_Checksum crc8;
    crc8.update(payload, payload_len);
    uint8_t sum    = crc8.value();
    auto crc_items = encodeManchester(&sum, 1);

    // Build frame: preamble + SOF + CRC8 + ID + Count + Length + Payload + EOF
    buf.insert(buf.end(), std::begin(preamble_array), std::end(preamble_array));

    constexpr m5_rmt_item_t sof[2] = {rmt_sof_0, rmt_sof_1};
    buf.insert(buf.end(), std::begin(sof), std::end(sof));

    buf.insert(buf.end(), crc_items.begin(), crc_items.end());
    buf.insert(buf.end(), id_items.begin(), id_items.end());
    buf.insert(buf.end(), count_items.begin(), count_items.end());
    buf.insert(buf.end(), len_items.begin(), len_items.end());
    buf.insert(buf.end(), payload_items.begin(), payload_items.end());

    buf.push_back(rmt_eof);

    return buf;
}

bool M5Codec::decode(const gpio::m5_rmt_item_t* items, uint32_t num, uint8_t* work_buf, uint16_t work_buf_size,
                     DecodeResult& result)
{
    // Scan for SOF, retry on false positives
    for (uint32_t sof_idx = 0; sof_idx < num; ++sof_idx) {
        if (!is_near(items[sof_idx].duration0, sof_expected_d0, sof_margin) ||
            !is_near(items[sof_idx].duration1, sof_expected_d1, sof_margin)) {
            continue;
        }

        uint32_t data_start = sof_idx + 1;
        uint32_t data_items = num - data_start;
        if (data_items < 2) {
            continue;
        }

        uint16_t dlen = decodeManchester(work_buf, work_buf_size, items + data_start, data_items);

        // Minimum: CRC(1) + ID(1) + Count(1) + Length(1) = 4 bytes
        if (dlen < 4) {
            continue;
        }

        // Parse header: CRC8(1) + ID(1) + Count(1) + Length(1)
        const uint8_t read_crc       = work_buf[0];
        const uint8_t id             = work_buf[1];
        const uint8_t count          = work_buf[2];
        const uint8_t payload_length = work_buf[3];
        const uint16_t payload_off   = 4;

        if (payload_off + payload_length > dlen) {
            continue;
        }

        // Verify CRC8 (payload only)
        m5::utility::CRC8_Checksum crc8;
        uint8_t calc_crc = crc8.range(work_buf + payload_off, payload_length);

        if (calc_crc != read_crc) {
            M5_LIB_LOGD("CRC mismatch at sof@%u: calc=0x%02X read=0x%02X", sof_idx, calc_crc, read_crc);
            continue;
        }

        result.id             = id;
        result.send_count     = count;
        result.payload_length = payload_length;
        result.payload_offset = payload_off;
        return true;
    }
    return false;
}

}  // namespace rf433
}  // namespace unit
}  // namespace m5

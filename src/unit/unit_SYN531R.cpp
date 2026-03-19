/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SYN531R.cpp
  @brief SYN531R unit for M5UnitUnified
*/
#include "unit_SYN531R.hpp"
#include "rmt_item_types.hpp"
#include <esp_heap_caps.h>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit;
using namespace m5::unit::gpio;
using namespace m5::unit::rf433;

namespace {

constexpr uint16_t expected_width0{2480};
constexpr uint16_t expected_width1{1630};
constexpr uint16_t margin0{250};
constexpr uint16_t margin1{250};

inline bool is_unit_pbhub(Component* u)
{
    static constexpr types::uid_t pbhub_uid{"UnitPbHub"_mmh3};
    return u->identifier() == pbhub_uid;
}

inline bool is_near(const uint16_t val, const uint16_t target, const uint16_t margin)
{
    return (val >= (target - margin)) && (val <= (target + margin));
}

}  // namespace

namespace m5 {
namespace unit {

const char UnitSYN531R::name[] = "UnitSYN531R";
const types::uid_t UnitSYN531R::uid{"UnitSYN531R"_mmh3};
const types::attr_t UnitSYN531R::attr{attribute::AccessGPIO};

bool UnitSYN531R::begin()
{
    // Search PbHub
    Component* p = this;
    while (p) {
        p = p->parent();
        if (p && is_unit_pbhub(p)) {
            break;
        }
    }
    if (p && is_unit_pbhub(p)) {
        M5_LIB_LOGE("Not support via PbHub");
        return false;
    }

    // Clamp max_payload_size to platform theoretical limit
    if (_cfg.max_payload_size > MaxPayloadSize) {
        M5_LIB_LOGW("max_payload_size %u exceeds platform max %u, clamped", _cfg.max_payload_size, MaxPayloadSize);
        _cfg.max_payload_size = MaxPayloadSize;
    }

    // Calculate ring buffer size: must hold at least 2048 bytes (empirically safe for AGC noise + data)
    uint16_t payload_ring = calculateRingBufferSize(_cfg.max_payload_size, _cfg.protocol);
    uint16_t buf_bytes    = payload_ring > 2048 ? payload_ring : 2048;
    buf_bytes             = (buf_bytes + 3) & ~3;  // 4-byte align (required by RMT v2)

    // Allocate 4-byte aligned receive buffer (required by RMT v2)
    auto* rx_buf = static_cast<uint8_t*>(heap_caps_aligned_alloc(4, buf_bytes, MALLOC_CAP_8BIT));
    if (!rx_buf) {
        M5_LIB_LOGE("Failed to allocate rx buffer (%u bytes)", buf_bytes);
        return false;
    }
    _rx_buffer.reset(rx_buf);
    _rx_buffer_size = buf_bytes;

    // Reserve memory for decoded data
    _data.reserve(buf_bytes >> 3);

    if (!pinModeRX(gpio::Mode::Input)) {
        return false;
    }

    adapter_config_t cfg{};
    cfg.mode       = Mode::RmtRX;
    cfg.rx.tick_ns = 1000;  // 1 tick = 1us
#if defined(M5_UNIT_UNIFIED_USING_RMT_V2)
    cfg.rx.mem_blocks = 2;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    // ESP32-S3 + ESP-IDF 4.x: RMT RX ping-pong has a known bug that causes memory corruption
    // and crash in noisy RF environments. See: https://github.com/espressif/esp-idf/issues/13419
    // Using mem_blocks=1 to minimize exposure, but crashes may still occur.
    // Recommended: use ESP-IDF 5.x (pioarduino) which has RMT v2 with the fix.
    M5_LIB_LOGW(
        "ESP32-S3 + ESP-IDF 4.x: RMT RX may crash in noisy environments (esp-idf#13419). "
        "Consider using ESP-IDF 5.x (pioarduino)");
    cfg.rx.mem_blocks = 1;
#else
    cfg.rx.mem_blocks = 6;
#endif
    cfg.rx.ring_buffer_size       = buf_bytes;
    cfg.rx.filter_enabled         = true;
    cfg.rx.filter_ticks_threshold = 200;
    cfg.rx.idle_ticks_threshold   = 3000;

    M5_LIB_LOGI("RX config: max_payload=%u mem_blocks=%u ring_buf=%u MaxPayloadSize=%u RmtRxMaxItems=%u",
                _cfg.max_payload_size, cfg.rx.mem_blocks, buf_bytes, MaxPayloadSize, RmtRxMaxItems);

    auto ad = asAdapter<AdapterGPIO>(Adapter::Type::GPIO);
    if (!ad || !ad->begin(cfg)) {
        M5_LIB_LOGE("Failed to begin AdapterGPIO");
        return false;
    }
    return true;
}

void UnitSYN531R::update(const bool /*force*/)
{
    auto now = m5::utility::millis();
    _updated = read_data();
    if (_updated) {
        _latest = now;
    }
}

bool UnitSYN531R::read_data()
{
    auto buffer_size = _rx_buffer_size;
    auto* buff       = _rx_buffer.get();

    if (readWithTransaction(buff, buffer_size) != m5::hal::error::error_t::OK) {
        return false;
    }

    uint16_t len  = *(uint16_t*)buff;
    uint16_t inum = len / sizeof(m5_rmt_item_t);
    if (inum < 2) {
        return false;
    }

    m5::unit::gpio::m5_rmt_item_t* items = (m5::unit::gpio::m5_rmt_item_t*)(buff + 2 /* len */);

    // Scan for SOF pattern, retry on false positives (AGC noise may match SOF)
    for (uint16_t sof_idx = 0; sof_idx < inum; ++sof_idx) {
        if (!is_near(items[sof_idx].duration0, expected_width0, margin0) ||
            !is_near(items[sof_idx].duration1, expected_width1, margin1)) {
            continue;
        }

        // Decode from after SOF
        uint16_t data_start = sof_idx + 1;
        uint16_t data_items = inum - data_start;
        if (data_items < 2) {
            continue;
        }

        constexpr uint16_t DECODE_BUF_SIZE = 264;
        uint8_t decode_data[DECODE_BUF_SIZE]{};
        uint16_t dlen = decodeManchester(decode_data, DECODE_BUF_SIZE, items + data_start, data_items);

        if (dlen < 4) {
            continue;
        }

        // Check sum
        const uint8_t read_sum       = decode_data[0];
        const Protocol prot          = decode_data[1];
        const uint8_t payload_offset = 3 + ((prot & ProtocolIncludeSendCount) ? 1 : 0) +
                                       ((prot & ProtocolIncludeIdentifier) ? sizeof(communication_identifier_t) : 0);

        if (payload_offset >= dlen) {
            continue;  // False SOF match, try next
        }

        m5::utility::CRC8_Checksum crc8{};
        const uint8_t sum = crc8.range(decode_data + payload_offset, dlen - payload_offset);

        if (sum == read_sum) {
            _data.insert(_data.end(), decode_data + 1, decode_data + dlen);
            return true;
        }
        M5_LIB_LOGD("CRC mismatch at sof@%u: calc=0x%02X read=0x%02X", sof_idx, sum, read_sum);
    }
    return false;
}

}  // namespace unit
}  // namespace m5

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

    auto buffer_size = component_config().stored_size;

    // Reserve memory
    _data.reserve(buffer_size >> 3);

    if (!pinModeRX(gpio::Mode::Input)) {
        return false;
    }

    adapter_config_t cfg{};
    cfg.mode       = Mode::RmtRX;
    cfg.rx.tick_ns = 1000;  // 1 tick = 1us
#if defined(M5_UNIT_UNIFIED_USING_RMT_V2) && defined(CONFIG_IDF_TARGET_ESP32C6)
    cfg.rx.mem_blocks = 2;
#else

#if defined(CONFIG_IDF_TARGET_ESP32S3)
    cfg.rx.mem_blocks = 4;
#else
    cfg.rx.mem_blocks = 6;
#endif
#endif
    cfg.rx.ring_buffer_size       = buffer_size;
    cfg.rx.filter_enabled         = true;
    cfg.rx.filter_ticks_threshold = 200;
    cfg.rx.idle_ticks_threshold   = 3000;

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
    auto buffer_size = component_config().stored_size;

    uint8_t buff[buffer_size]{};

    if (readWithTransaction(buff, buffer_size) == m5::hal::error::error_t::OK) {
        uint16_t len  = *(uint16_t*)buff;
        uint16_t inum = len / sizeof(m5_rmt_item_t);
        if (inum < 2) {
            return false;
        }

        // SOF + SUM + Protocol[1] + [identifier 4 if exists] [send cout 1 if exists ] [payload length] [payload n]
        m5::unit::gpio::m5_rmt_item_t* items = (m5::unit::gpio::m5_rmt_item_t*)(buff + 2 /* len */);

        // Check SOF
        if (!is_near(items[0].duration0, expected_width0, margin0) ||
            !is_near(items[0].duration1, expected_width1, margin1)) {
            M5_LIB_LOGD("Invalid SOF pattern %u (%u,%u)", inum, items[0].duration0, items[0].duration1);
            return false;
        }

        // Decode
        uint8_t decode_data[buffer_size >> 3]{};
        uint16_t dlen = decodeManchester(decode_data, buffer_size >> 3, items + 1, inum - 1);

        // M5_LIB_LOGE(">>>> i:%u d:%u", inum, dlen);

        if (dlen < 4) {
            return false;
        }

        // m5::utility::log::dump(_data.data(), _dlen,false);

        // Check sum
        const uint8_t read_sum       = decode_data[0];
        const Protocol prot          = decode_data[1];
        const uint8_t payload_offset = 3 + ((prot & ProtocolIncludeSendCount) ? 1 : 0) +
                                       ((prot & ProtocolIncludeIdentifier) ? sizeof(communication_identifier_t) : 0);
        // M5_LIB_LOGD("RS:%u PR:%x off:%u", read_sum, prot, payload_offset);

        if (payload_offset >= dlen) {
            M5_LIB_LOGE("Invalid offset %u/%zu", payload_offset, dlen);
            return false;
        }

        m5::utility::CRC8_Checksum crc8{};
        const uint8_t sum = crc8.range(decode_data + payload_offset, dlen - payload_offset);
        // M5_LIB_LOGD("off:%u read_sum:%u sum:%u", payload_offset, read_sum, sum);

        if (sum == read_sum && payload_offset < dlen) {
            _data.insert(_data.end(), decode_data + 1, decode_data + dlen);
            return true;
        }
    }
    return false;
}

}  // namespace unit
}  // namespace m5

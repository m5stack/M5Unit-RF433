/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SYN115.cpp
  @brief SYN115 unit for M5UnitUnified
*/
#include "unit_SYN115.hpp"
#include "rmt_item_types.hpp"
#include <M5Utility.hpp>

using namespace m5::utility::mmh3;
using namespace m5::unit::types;
using namespace m5::unit;
using namespace m5::unit::gpio;
using namespace m5::unit::rf433;

namespace {

inline bool is_unit_pbhub(Component* u)
{
    static constexpr types::uid_t pbhub_uid{"UnitPbHub"_mmh3};
    return u->identifier() == pbhub_uid;
}

constexpr m5::unit::gpio::m5_rmt_item_t rmt_eof{{1000 * 5, 0, 1000 * 5, 0}};
constexpr m5_rmt_item_t preamble_array[] = {rmt_preamble, rmt_preamble, rmt_preamble, rmt_preamble, rmt_preamble,
                                            rmt_preamble, rmt_preamble, rmt_preamble, rmt_preamble, rmt_preamble};

}  // namespace

namespace m5 {
namespace unit {

const char UnitSYN115::name[] = "UnitSYN115";
const types::uid_t UnitSYN115::uid{"UnitSYN115"_mmh3};
const types::attr_t UnitSYN115::attr{attribute::AccessGPIO};

bool UnitSYN115::begin()
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

    if (!pinModeTX(gpio::Mode::Output)) {
        return false;
    }

    adapter_config_t cfg{};
    cfg.mode                   = Mode::RmtTX;
    cfg.tx.tick_ns             = 1000;
    cfg.tx.mem_blocks          = 1;
    cfg.tx.idle_output_enabled = true;
    cfg.tx.idle_level_high     = false;
    cfg.tx.with_dma            = false;
    cfg.tx.loop_enabled        = false;

    auto ad = asAdapter<AdapterGPIO>(Adapter::Type::GPIO);
    if (!ad || !ad->begin(cfg)) {
        M5_LIB_LOGE("Failed to begin AdapterGPIO");
        return false;
    }

    clear_rmt_buffer();
    return true;
}

void UnitSYN115::update(const bool force)
{
    if (!_rmt_buffer.empty() && _cfg.send_in_update) {
        if (!send(_cfg.burst_transmission_count)) {
            M5_LIB_LOGD("Failed to send %zu", _rmt_buffer.size());
        }
    }
}

bool UnitSYN115::push_back(const uint8_t* data, const uint32_t len)
{
    if (_payload_size + len > 255) {
        M5_LIB_LOGE("Not enough payload (max 255) %u/%u", _payload_size, len);
        return false;
    }

    if (!_closing) {
        _crc8.update(data, len);
        auto ev = encodeManchester(data, len);
        _rmt_buffer.insert(_rmt_buffer.end(), ev.begin(), ev.end());
        _payload_size += len;
        return true;
    }
    M5_LIB_LOGD("push_back rejected because _closing=true");
    return false;
}

bool UnitSYN115::send(const uint8_t burst_transmission_count)
{
    if (_rmt_buffer.empty()) {
        return false;
    }

    // preamble + SOF + SUM + Protocol[1] + [identifier 4 if exists] [send cout 1 if exists ] [payload length] [payload
    // n]
    if (!_closing) {
        // Add Length of payload (before encode)
        {
            uint8_t ps = _payload_size;
            auto sv    = encodeManchester(&ps, 1);
            _rmt_buffer.insert(_rmt_buffer.begin(), sv.begin(), sv.end());
        }
        // Add send count (1)
        if (_cfg.protocol & ProtocolIncludeSendCount) {
            auto sv = encodeManchester(&_send_count, 1);
            _rmt_buffer.insert(_rmt_buffer.begin(), sv.begin(), sv.end());
            ++_send_count;
        }

        // Add identifier (4)
        if (_cfg.protocol & ProtocolIncludeIdentifier) {
            auto sv = encodeManchester((uint8_t*)&_comm_id, sizeof(_comm_id));
            _rmt_buffer.insert(_rmt_buffer.begin(), sv.begin(), sv.end());
        }

        // Protocol (1)
        {
            auto sv = encodeManchester(&_cfg.protocol, 1);
            _rmt_buffer.insert(_rmt_buffer.begin(), sv.begin(), sv.end());
        }

        // CheckSum (1)
        {
            const uint8_t sum = _crc8.value();  // only payload
            auto sv           = encodeManchester(&sum, 1);
            _rmt_buffer.insert(_rmt_buffer.begin(), sv.begin(), sv.end());
        }

        // SOF
        constexpr m5_rmt_item_t sof[2] = {rmt_sof_0, rmt_sof_1};
        _rmt_buffer.insert(_rmt_buffer.begin(), std::begin(sof), std::end(sof));

        // preamble
        _rmt_buffer.insert(_rmt_buffer.begin(), std::begin(preamble_array), std::end(preamble_array));

        // EOF
        _rmt_buffer.push_back(rmt_eof);

        _closing = true;
    }

    //    auto wait = estimate_tx_timeout_ticks();
    auto wait     = portMAX_DELAY;
    uint8_t count = burst_transmission_count ? burst_transmission_count : 1;
    bool ret{true};

    // Burst transmission
    while (ret && count--) {
        ret &= (writeWithTransaction((const uint8_t*)_rmt_buffer.data(), _rmt_buffer.size() * sizeof(m5_rmt_item_t),
                                     wait) == m5::hal::error::error_t::OK);
    };
    if (ret) {
        clear_rmt_buffer();
    }
    return ret;
}

void UnitSYN115::clear_rmt_buffer()
{
    _rmt_buffer.clear();
    _crc8.clear();
    _payload_size = 0;
    _closing      = false;
}

TickType_t UnitSYN115::estimate_tx_timeout_ticks(const uint32_t margin_ms) const
{
    uint32_t total_us{};
    for (const auto& item : _rmt_buffer) {
        total_us += item.duration0 + item.duration1;
    }
    total_us += margin_ms * 1000;
    return pdMS_TO_TICKS((total_us + 999) / 1000);  // us -> ms -> tick
}

}  // namespace unit
}  // namespace m5

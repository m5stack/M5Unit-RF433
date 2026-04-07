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

    clear();
    return true;
}

void UnitSYN115::update(const bool force)
{
    (void)force;
    if (!_payload.empty() && _cfg.send_in_update) {
        if (!send(_cfg.burst_transmission_count)) {
            M5_LIB_LOGD("Failed to send");
        }
    }
}

bool UnitSYN115::push_back(const uint8_t* data, const uint32_t len)
{
    if (!data || len == 0) {
        return false;
    }
    if (_payload_size + len > 255) {
        M5_LIB_LOGE("Payload exceeds max (255 bytes): %u + %u", _payload_size, len);
        return false;
    }

    _payload.insert(_payload.end(), data, data + len);
    _payload_size += len;
    return true;
}

bool UnitSYN115::send(const uint8_t burst_transmission_count)
{
    if (_payload.empty()) {
        return false;
    }

    // Encode complete frame via codec
    auto rmt_items = _codec->encode(_payload.data(), _payload_size);

    auto wait     = estimate_tx_timeout_ticks(rmt_items);
    uint8_t count = burst_transmission_count ? burst_transmission_count : _cfg.burst_transmission_count;
    bool ret{true};

    // Burst transmission
    while (ret && count--) {
        ret &= (writeWithTransaction(reinterpret_cast<const uint8_t*>(rmt_items.data()),
                                     rmt_items.size() * sizeof(m5_rmt_item_t), wait) == m5::hal::error::error_t::OK);
    }
    if (ret) {
        clear();
    }
    return ret;
}

TickType_t UnitSYN115::estimate_tx_timeout_ticks(const rf433::item_container_type& items,
                                                 const uint32_t margin_ms) const
{
    uint32_t total_us{};
    for (const auto& item : items) {
        total_us += item.duration0 + item.duration1;
    }
    total_us += margin_ms * 1000;
    return pdMS_TO_TICKS((total_us + 999) / 1000);  // us -> ms -> tick
}

}  // namespace unit
}  // namespace m5

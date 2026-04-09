/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file rmt_item_types.hpp
  @brief RMT related definition and function for RF433
*/
#ifndef M5_UNIT_RF433_RMT_ITEM_TYPES_HPP
#define M5_UNIT_RF433_RMT_ITEM_TYPES_HPP

#include <M5UnitComponent.hpp>

namespace m5 {
namespace unit {
namespace rf433 {

///@cond
constexpr m5::unit::gpio::m5_rmt_item_t rmt_item_one{{800, 1, 200, 0}};   // 1
constexpr m5::unit::gpio::m5_rmt_item_t rmt_item_zero{{200, 1, 800, 0}};  // 0
constexpr m5::unit::gpio::m5_rmt_item_t rmt_sof_0{{4868, 1, 2469, 0}};    // Frame start 0
constexpr m5::unit::gpio::m5_rmt_item_t rmt_sof_1{{1647, 1, 315, 0}};     // Frame start 1
constexpr m5::unit::gpio::m5_rmt_item_t rmt_preamble{{500, 1, 500, 0}};   // preamble
///@endcond

using container_type             = std::vector<uint8_t>;              //!< Container
using item_container_type        = std::vector<gpio::m5_rmt_item_t>;  //!< Item container
using communication_identifier_t = uint8_t;                           //!< Communication identifier (0-255)

//! @brief Protocol overhead in bytes: CRC8(1) + ID(1) + Count(1) + Length(1) = 4
constexpr uint8_t ProtocolOverhead = 4;

/*!
  @brief Maximum RMT items receivable in a single frame per platform
  @details
  - ESP32 (RMT v1): 6 mem_blocks x 64 = 384 items
  - ESP32-S3 (RMT v1): 1 mem_block x 48 = 48 items (threshold ISR wrapping)
  - ESP-IDF 5.x (RMT v2): ping-pong/DMA, limited by user buffer only
  @note RF433 ASK receivers (SYN531R) generate AGC noise before the SOF, consuming part of the RMT memory.
  The maximum payload must fit within a single RMT hardware frame to avoid truncation.
  @warning ESP32-S3 + ESP-IDF 4.x has a known RMT RX ping-pong bug that may cause memory corruption
  and crash in noisy RF environments. See https://github.com/espressif/esp-idf/issues/13419
  Recommended to use ESP-IDF 5.x (pioarduino) for ESP32-S3.
 */
#if defined(M5_UNIT_UNIFIED_USING_RMT_V2)
constexpr uint16_t RmtRxMaxItems = 4096;  //!< RMT v2: ping-pong/DMA handles large frames
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr uint16_t RmtRxMaxItems = 1 * 48;  //!< ESP32-S3: 1 mem_block x 48 items (threshold ISR handles wrapping)
#else
constexpr uint16_t RmtRxMaxItems = 6 * 64;  //!< ESP32: 8ch x 64 items, use 6 = 384
#endif

/*!
  @brief Theoretical maximum payload size in bytes for the current platform
  @details Calculated from RmtRxMaxItems with protocol overhead (4 bytes).
  This is the theoretical limit based on RMT hardware memory. In practice, AGC noise from the
  SYN531R receiver consumes RMT items, reducing the usable capacity.
  Theoretical values:
  - ESP32: 43 bytes (practical safe limit ~23 bytes)
  - ESP32-S3: 43 bytes (1 mem_block + threshold ISR wrapping; practical safe limit ~23 bytes)
  - ESP-IDF 5.x (RMT v2): 255 bytes
  @warning When communicating between RMT v1 (ESP-IDF 4.x) and RMT v2 (ESP-IDF 5.x) devices,
  the payload size must not exceed the receiver's limit. A v2 transmitter can send up to 255 bytes,
  but a v1 receiver's capacity is much smaller and varies depending on AGC noise conditions.
  Always limit the payload to the receiver's max_payload_size.
  @see UnitSYN531R::config_t::max_payload_size for the configurable runtime limit
 */
constexpr uint8_t MaxPayloadSize =
#if defined(M5_UNIT_UNIFIED_USING_RMT_V2)
    255;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    // ESP32-S3 uses mem_blocks=1 with threshold ISR wrapping;
    // theoretical limit depends on ISR throughput, use ESP32 equivalent
    43;
#else
    static_cast<uint8_t>((RmtRxMaxItems - 1 /*SOF*/) / 8 - ProtocolOverhead);
#endif

/*!
  @brief Calculate the minimum ring buffer size required to receive the given payload size
  @param payload_size Payload size in bytes
  @return Required ring buffer size in bytes
 */
constexpr uint32_t calculateRingBufferSize(const uint32_t payload_size)
{
    // Frame: SOF(1 item) + (overhead + payload) * 8 Manchester items per byte
    return (1 /*SOF*/ + (ProtocolOverhead + payload_size) * 8) * sizeof(m5::unit::gpio::m5_rmt_item_t) +
           2 /*length prefix*/;
}

/*!
  @brief Encode manchester
  @param data Input buffer
  @param len Length of input buffer
  @param MSB Process from MSB if true
  @return Encoded container
 */
item_container_type encodeManchester(const uint8_t* data, const uint32_t len, const bool MSB = true);

/*!
  @brief Decode manchester
  @param buf Output buffer
  @param buf_size Output buffer size
  @param data RMT data (exclude SOF)
  @param num Number of the RMT items
  @param MSB Process from MSB if true
  @return Decoded count
 */
uint16_t decodeManchester(uint8_t* buf, const uint16_t buf_size, const m5::unit::gpio::m5_rmt_item_t* data,
                          const uint32_t num, const bool MSB = true);

}  // namespace rf433
}  // namespace unit
}  // namespace m5
#endif

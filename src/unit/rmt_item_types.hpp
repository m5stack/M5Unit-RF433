/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file rmt_item_types.hpp
  @brief RMT releated definition and function for RF433
*/
#ifndef M5_UNIT_RF433_RNT_ITEM_TYPES_HPP
#define M5_UNIT_RF433_RNT_ITEM_TYPES_HPP

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
using communication_identifier_t = uint32_t;                          //!< Communication identifier

///@name Protocol attribute bits
///@{
using Protocol = uint8_t;                            //!< Protocol type
constexpr Protocol ProtocolIncludeSendCount{0x01};   //!< Include send count
constexpr Protocol ProtocolIncludeIdentifier{0x02};  //!< Include identifier
///@}

/*!
  @brief Encode manchester
  @param data Input buffer
  @param Length of input buffer
  @param MSB Process from MSB if true
  @return Encoded container
 */
item_container_type encodeManchester(const uint8_t* data, const uint32_t len, const bool MSB = true);

/*!
  @brief Decode manchester
  @param buf Output buffer
  @paran buf_sizr Output buffer size
  @param data RMT data (exclude SOF)
  @param Number of the RMT items
  @param MSB Process from MSB if true
  @return Decoded count
 */
uint16_t decodeManchester(uint8_t* buf, const uint16_t buf_size, const m5::unit::gpio::m5_rmt_item_t* data,
                          const uint32_t num, const bool MSB = true);

}  // namespace rf433
}  // namespace unit
}  // namespace m5
#endif

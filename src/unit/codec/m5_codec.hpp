/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file m5_codec.hpp
  @brief M5Unit-RF433 original protocol codec
  @details Frame format:
  preamble(10) + SOF(2) + CRC8(1) + ID(1) + Count(1) + Length(1) + Payload(n) + EOF(1)
  Manchester encoding, MSB first.
*/
#ifndef M5_UNIT_RF433_CODEC_M5_CODEC_HPP
#define M5_UNIT_RF433_CODEC_M5_CODEC_HPP

#include "protocol_codec.hpp"
#include <m5_utility/crc.hpp>

namespace m5 {
namespace unit {
namespace rf433 {

/*!
  @class M5Codec
  @brief M5Unit-RF433 original protocol codec
  @details Always includes communication identifier (1 byte) and send count (1 byte).
  CRC8 covers payload only.
 */
class M5Codec : public ProtocolCodec {
public:
    M5Codec() : ProtocolCodec(CodecType::M5RF433)
    {
    }

    /*!
      @brief Encode payload into RMT items with M5 protocol framing
      @param payload Pointer to the payload data
      @param payload_len Length of the payload in bytes
      @return Encoded RMT items including preamble, SOF, protocol fields, and Manchester-encoded data
     */
    item_container_type encode(const uint8_t* payload, uint32_t payload_len) override;

    /*!
      @brief Decode RMT items into payload data
      @param items Pointer to the received RMT items (after SOF detection)
      @param num Number of RMT items
      @param work_buf Working buffer for decoded bytes
      @param work_buf_size Size of the working buffer
      @param[out] result Decoded result containing payload pointer and metadata
      @return True if decoding and CRC8 verification succeeded
     */
    bool decode(const gpio::m5_rmt_item_t* items, uint32_t num, uint8_t* work_buf, uint16_t work_buf_size,
                DecodeResult& result) override;

    /*!
      @brief Get protocol overhead in bytes
      @return 4 (CRC8 + ID + Count + Length)
     */
    uint8_t overhead() const override
    {
        // CRC8(1) + ID(1) + Count(1) + Length(1) = 4
        return 4;
    }

    ///@name Communication identifier (M5Codec specific)
    ///@{
    /*!
      @brief Get the communication identifier
      @return Current communication identifier (0-255)
     */
    inline communication_identifier_t communicationIdentifier() const
    {
        return _comm_id;
    }
    /*!
      @brief Set the communication identifier
      @param id Communication identifier (0-255) for filtering received frames
     */
    inline void setCommunicationIdentifier(communication_identifier_t id)
    {
        _comm_id = id;
    }
    ///@}

private:
    communication_identifier_t _comm_id{};
    uint8_t _send_count{};
};

}  // namespace rf433
}  // namespace unit
}  // namespace m5
#endif

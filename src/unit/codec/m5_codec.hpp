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

    item_container_type encode(const uint8_t* payload, uint32_t payload_len) override;

    bool decode(const gpio::m5_rmt_item_t* items, uint32_t num, uint8_t* work_buf, uint16_t work_buf_size,
                DecodeResult& result) override;

    uint8_t overhead() const override
    {
        // CRC8(1) + ID(1) + Count(1) + Length(1) = 4
        return 4;
    }

    ///@name Communication identifier (M5Codec specific)
    ///@{
    inline communication_identifier_t communicationIdentifier() const
    {
        return _comm_id;
    }
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

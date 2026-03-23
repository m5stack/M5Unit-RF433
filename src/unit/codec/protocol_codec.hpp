/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file protocol_codec.hpp
  @brief Protocol codec base class for RF433
*/
#ifndef M5_UNIT_RF433_CODEC_PROTOCOL_CODEC_HPP
#define M5_UNIT_RF433_CODEC_PROTOCOL_CODEC_HPP

#include "../rmt_item_types.hpp"

namespace m5 {
namespace unit {
namespace rf433 {

/*!
  @struct DecodeResult
  @brief Result of decoding a received frame
 */
struct DecodeResult {
    communication_identifier_t id{};  //!< Communication identifier
    uint8_t send_count{};             //!< Send count (for duplicate detection)
    uint8_t payload_length{};         //!< Payload length in bytes
    uint16_t payload_offset{};        //!< Offset to payload in decode buffer
};

/*!
  @enum CodecType
  @brief Identifies the protocol codec implementation
  @details To create a custom protocol, derive from ProtocolCodec with CodecType::Custom
  and implement encode()/decode()/overhead(). See M5Codec (m5_codec.hpp) as a reference.
 */
enum class CodecType : uint8_t {
    M5RF433 = 0,  //!< M5Unit-RF433 original protocol (@see M5Codec)
    // Future protocol support (not yet implemented):
    // EV1527,     //!< EV1527 / HS1527 / RT1527 (24-bit code + sync)
    // PT2262,     //!< PT2262 / PT2272 (tri-state encoding)
    // RadioHead,  //!< RadioHead / VirtualWire ASK compatible
    Custom = 255,  //!< User-defined custom protocol
};

/*!
  @class ProtocolCodec
  @brief Abstract base class for RF433 protocol encoding/decoding
 */
class ProtocolCodec {
public:
    explicit ProtocolCodec(CodecType t) : _type(t)
    {
    }
    virtual ~ProtocolCodec() = default;

    //! @brief Get codec type for safe downcasting
    CodecType type() const
    {
        return _type;
    }

    ///@name TX
    ///@{
    /*!
      @brief Encode payload into a complete RMT frame (preamble + SOF + header + data + EOF)
      @param payload Payload data
      @param payload_len Payload length in bytes
      @return RMT items for transmission
     */
    virtual item_container_type encode(const uint8_t* payload, uint32_t payload_len) = 0;
    ///@}

    ///@name RX
    ///@{
    /*!
      @brief Try to decode a valid frame from RMT items
      @param items RMT items from receiver
      @param num Number of items
      @param work_buf Working buffer for Manchester decoding
      @param work_buf_size Size of working buffer
      @param[out] result Decoded frame result
      @return true if a valid frame was found and decoded
     */
    virtual bool decode(const gpio::m5_rmt_item_t* items, uint32_t num, uint8_t* work_buf, uint16_t work_buf_size,
                        DecodeResult& result) = 0;
    ///@}

    ///@name Properties
    ///@{
    /*!
      @brief Maximum protocol overhead in bytes (excluding payload)
      @return Overhead size
     */
    virtual uint8_t overhead() const = 0;
    ///@}

private:
    CodecType _type;
};

}  // namespace rf433
}  // namespace unit
}  // namespace m5
#endif

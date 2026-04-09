/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SYN115.hpp
  @brief SYN115 unit for M5UnitUnified
*/
#ifndef M5_UNIT_RF433_UNIT_SYN115_HPP
#define M5_UNIT_RF433_UNIT_SYN115_HPP

#include <M5UnitComponent.hpp>
#include <memory>
#include "codec/m5_codec.hpp"

namespace m5 {
namespace unit {

/*!
  @class m5::unit::UnitSYN115
  @brief SYN115 unit
  @details RF433 Transmitter
*/
class UnitSYN115 : public Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSYN115, 0x00);

public:
    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
        //! If true, pushed data is automatically sent during update(). If false, call send() explicitly.
        bool send_in_update{false};
        //! Count of burst transmission
        uint8_t burst_transmission_count{2};
    };

    ///@name Configuration for begin
    ///@{
    /*! @brief Gets the configuration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configuration
    inline void config(const config_t& cfg)
    {
        _cfg = cfg;
    }
    ///@}

    UnitSYN115() : Component(0x00)
    {
    }
    virtual ~UnitSYN115()
    {
    }

    //! @brief Initialize the transmitter unit
    virtual bool begin() override;
    //! @brief Update the transmitter unit
    virtual void update(const bool force = false) override;

    //! @brief Get codec (for codec-specific configuration)
    inline std::shared_ptr<rf433::ProtocolCodec> codec()
    {
        return _codec;
    }

    /*!
      @brief Push back data to payload
      @param data Input data buffer
      @param len Length of data buffer
      @return True if successful
      @warning Total payload size is limited to 255 bytes.
      The receiver side has a stricter limit based on RMT hardware (rf433::MaxPayloadSize).
      @see rf433::MaxPayloadSize
     */
    bool push_back(const uint8_t* data, const uint32_t len);

    /*!
      @brief Send force if exists payload
      @param burst_transmission_count Count of burst transmission (0 = use config_t::burst_transmission_count)
      @return True if successful
      @note The payload will be empty if successful
     */
    bool send(const uint8_t burst_transmission_count = 0);

    /*!
      @brief Clear inner buffer
     */
    inline void clear()
    {
        _payload.clear();
        _payload_size = 0;
    }

    //! @brief Set protocol codec (default: M5Codec)
    void setCodec(std::shared_ptr<rf433::ProtocolCodec> codec)
    {
        _codec = codec;
    }

protected:
    TickType_t estimate_tx_timeout_ticks(const rf433::item_container_type& items, const uint32_t margin_ms = 10) const;

private:
    std::shared_ptr<rf433::ProtocolCodec> _codec{std::make_shared<rf433::M5Codec>()};
    std::vector<uint8_t> _payload{};
    uint16_t _payload_size{};
    config_t _cfg{};
};

}  // namespace unit
}  // namespace m5
#endif

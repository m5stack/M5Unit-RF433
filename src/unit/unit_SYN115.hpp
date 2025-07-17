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
#include <m5_utility/crc.hpp>
#include "rmt_item_types.hpp"

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
        //! Send in update() if true
        bool send_in_update{true};
        //! Count of burst transmission
        uint8_t burst_transmission_count{4};
        //! Protocol
        rf433::Protocol protocol{rf433::ProtocolIncludeSendCount | rf433::ProtocolIncludeIdentifier};
    };

    ///@name Configuration for begin
    ///@{
    /*! @brief Gets the configration */
    inline config_t config()
    {
        return _cfg;
    }
    //! @brief Set the configration
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

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Communication identifier
    ///@{
    //! @brief Get communication identifier
    inline rf433::communication_identifier_t communicationIdentifier() const
    {
        return _comm_id;
    }
    //! @brief Set communication identifier
    inline void setCommunicationIdentifier(rf433::communication_identifier_t id)
    {
        _comm_id = id;
    }
    ///@}

    /*!
      @param Push back to payload
      @param data Input data buffer
      @param len Length of data buffer
      @return True if successful
     */
    bool push_back(const uint8_t* data, const uint32_t len);

    /*!
      @brief Send force if exists payload
      @param burst_transmission_count Count of burst transmission
      @return True if successful
      @note The payload will be empty if successful
     */
    bool send(const uint8_t burst_transmission_count = 4);

    /*!
      @brief Clear inner buffer
     */
    inline void clear()
    {
        clear_rmt_buffer();
    }

protected:
    void clear_rmt_buffer();
    TickType_t estimate_tx_timeout_ticks(const uint32_t margin_ms = 10) const;

protected:
    m5::utility::CRC8_Checksum _crc8{};
    uint16_t _payload_size{};

private:
    rf433::item_container_type _rmt_buffer{};
    rf433::communication_identifier_t _comm_id{};
    bool _closing{};
    uint8_t _send_count{};  // Transmission Counter
    config_t _cfg{};
};

}  // namespace unit
}  // namespace m5
#endif

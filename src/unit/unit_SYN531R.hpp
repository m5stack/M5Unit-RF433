/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file unit_SYN531R.hpp
  @brief SYN531R unit for M5UnitUnified
*/
#ifndef M5_UNIT_RF433_UNIT_SYN531R_HPP
#define M5_UNIT_RF433_UNIT_SYN531R_HPP

#include <M5UnitComponent.hpp>
#include "rmt_item_types.hpp"
#include <vector>

namespace m5 {
namespace unit {

/*!
  @class m5::unit::UnitSYN531R
  @brief SYN531R unit
  @details RF433 Receiver
*/
class UnitSYN531R : public Component {
    M5_UNIT_COMPONENT_HPP_BUILDER(UnitSYN531R, 0x00);

public:
    using container_type = rf433::container_type;

    /*!
      @struct config_t
      @brief Settings for begin
     */
    struct config_t {
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

    UnitSYN531R() : Component(DEFAULT_ADDRESS)
    {
        auto ccfg        = component_config();
        ccfg.stored_size = 2048;  // inner buffer size
        component_config(ccfg);
    }
    virtual ~UnitSYN531R()
    {
    }

    virtual bool begin() override;
    virtual void update(const bool force = false) override;

    ///@name Data
    ///@{
    //! @brief Gets the number of stored data
    inline size_t available() const
    {
        return _data.size();
    }
    //! @brief Is empty stored data?
    inline bool empty() const
    {
        return _data.empty();
    }
    //! @brief Retrieve oldest stored data
    inline uint8_t oldest() const
    {
        return !_data.empty() ? _data.front() : 0;
    }
    //! @brief Retrieve latest stored data
    inline uint8_t latest() const
    {
        return !_data.empty() ? _data.back() : 0;
    }
    //! @brief Discard  the oldest data accumulated
    inline void discard()
    {
        if (!_data.empty()) {
            _data.erase(_data.begin());
        }
    }
    //! @brief Discard all data
    inline void flush()
    {
        _data.clear();
    }

    //! @brief Gets the received container reference
    inline const container_type& container() const
    {
        return _data;
    }
    ///@}

protected:
    bool read_data();

private:
    container_type _data{};
    config_t _cfg{};
};

}  // namespace unit
}  // namespace m5
#endif

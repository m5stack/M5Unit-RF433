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
#include <memory>
#include <vector>
#include "codec/m5_codec.hpp"

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
        //! Maximum receivable payload size in bytes
        //! @note Default is a conservative value safe for most environments.
        //! Theoretical max is rf433::MaxPayloadSize per platform, but AGC noise
        //! from the SYN531R receiver consumes RMT memory, reducing the practical limit.
        //! Exceeding the hardware capacity may cause data loss (ESP32) or crash (ESP32-S3).
        //! Increase at your own risk after testing in your environment.
        //! @note Practical safe defaults (tested): ESP32/ESP32-S3=23, RMT v2(ESP-IDF 5.x)=255
        //! @warning When communicating between RMT v1 and v2 devices, the transmitter's payload
        //! must not exceed the receiver's limit. The v1 RX capacity varies with AGC noise conditions.
        //! Test in your actual environment to determine the reliable maximum for your setup.
        uint8_t max_payload_size
        {
#if defined(M5_UNIT_UNIFIED_USING_RMT_V2)
            255
#else
            23
#endif
        };
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

    UnitSYN531R() : Component(DEFAULT_ADDRESS)
    {
    }
    virtual ~UnitSYN531R()
    {
    }

    //! @brief Initialize the receiver unit
    virtual bool begin() override;
    //! @brief Update the receiver unit
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
    //! @brief Discard the oldest data accumulated
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

    //! @brief Get codec (for codec-specific configuration)
    inline std::shared_ptr<rf433::ProtocolCodec> codec()
    {
        return _codec;
    }
    //! @brief Set protocol codec (default: M5Codec)
    void setCodec(std::shared_ptr<rf433::ProtocolCodec> codec)
    {
        _codec = codec;
    }

protected:
    bool read_data();

private:
    struct FreeDeleter {
        void operator()(uint8_t* p) const
        {
            free(p);
        }
    };
    std::shared_ptr<rf433::ProtocolCodec> _codec{std::make_shared<rf433::M5Codec>()};
    container_type _data{};
    std::unique_ptr<uint8_t[], FreeDeleter> _rx_buffer{};
    size_t _rx_buffer_size{};
    config_t _cfg{};
};

}  // namespace unit
}  // namespace m5
#endif

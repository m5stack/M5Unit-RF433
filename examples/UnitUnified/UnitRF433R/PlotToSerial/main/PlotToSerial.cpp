/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitRF433R
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedRF433.h>
#if __has_include(<esp_idf_version.h>)
#include <esp_idf_version.h>
#else  // esp_idf_version.h has been introduced in Arduino 1.0.5 (ESP-IDF3.3)
#define ESP_IDF_VERSION_VAL(major, minor, patch) ((major << 16) | (minor << 8) | (patch))
#define ESP_IDF_VERSION                          ESP_IDF_VERSION_VAL(3, 2, 0)
#endif

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitRF433R unit;
uint8_t latest_send_count{0xFF};

}  // namespace

void setup()
{
    M5.begin();

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }
    auto pin_num_gpio_in  = M5.getPin(m5::pin_name_t::port_b_in);
    auto pin_num_gpio_out = M5.getPin(m5::pin_name_t::port_b_out);
    if (pin_num_gpio_in < 0 || pin_num_gpio_out < 0) {
        M5_LOGW("PortB is not available");
        Wire.end();
        pin_num_gpio_in  = M5.getPin(m5::pin_name_t::port_a_pin1);
        pin_num_gpio_out = M5.getPin(m5::pin_name_t::port_a_pin2);
    }
    M5_LOGI("getPin: %d,%d", pin_num_gpio_in, pin_num_gpio_out);

    if (!Units.add(unit, pin_num_gpio_in, pin_num_gpio_out) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    // TAG specification by ESP_DRAM_LOGx does not work, so use wildcards
    esp_log_level_set("*", ESP_LOG_NONE); // Disable RMT warning log
    
    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
    M5_LOGI("ESP-IDF Version %d.%d.%d", (ESP_IDF_VERSION >> 16) & 0xFF, (ESP_IDF_VERSION >> 8) & 0xFF,
            ESP_IDF_VERSION & 0xFF);

    lcd.fillScreen(TFT_DARKGREEN);

    M5.Log.printf("getPin: %d,%d\n", pin_num_gpio_in, pin_num_gpio_out);
}

void loop()
{
    using namespace m5::unit::rf433;

    M5.update();
    // auto touch = M5.Touch.getDetail();
    Units.update();

    if (unit.updated()) {
        const auto& c = unit.container();
        // m5::utility::log::dump(c.data(), c.size(), false);

        auto prot = c[0];  // front is protocol
        uint32_t id{};
        uint8_t send_count = latest_send_count;
        uint32_t offset{1};

        if (prot & ProtocolIncludeIdentifier) {
            id = *(uint32_t*)(c.data() + offset);
            offset += 4;
        }
        if (prot & ProtocolIncludeSendCount) {
            send_count = c[offset++];
            // Skip duplicates due to burst transmission
            if (send_count == latest_send_count) {
                unit.flush();
                return;
            }
            latest_send_count = send_count;
        }
        uint8_t len = c[offset++];
        M5.Log.printf("RECEIVED: From<%X> Count:%u Len:%u [%s]\n", id, send_count, len,
                      (const char*)(c.data() + offset));
        lcd.fillRect(0, 0, lcd.width(), 8, 0);
        lcd.setCursor(0, 0);
        lcd.printf("%s", (const char*)(c.data() + offset));
        unit.flush();
    }
}

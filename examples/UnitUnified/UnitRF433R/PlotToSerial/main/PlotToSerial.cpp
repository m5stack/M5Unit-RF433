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
    M5.setTouchButtonHeightByRatio(100);

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
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    // TAG specification by ESP_DRAM_LOGx does not work, so use wildcards
    esp_log_level_set("*", ESP_LOG_NONE);  // Disable RMT warning log

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());
    M5_LOGI("ESP-IDF Version %d.%d.%d", (ESP_IDF_VERSION >> 16) & 0xFF, (ESP_IDF_VERSION >> 8) & 0xFF,
            ESP_IDF_VERSION & 0xFF);

    lcd.fillScreen(TFT_DARKCYAN);
    lcd.setCursor(0, 0);
    lcd.setTextSize(1);
    lcd.print("RX");

    M5.Log.printf("getPin: %d,%d\n", pin_num_gpio_in, pin_num_gpio_out);
}

void loop()
{
    M5.update();
    Units.update();

    if (unit.updated()) {
        const auto& c = unit.container();
        // Container format: ID(1) + Count(1) + Length(1) + Payload(n)
        if (c.size() < 3) {
            unit.flush();
            return;
        }

        uint8_t id         = c[0];
        uint8_t send_count = c[1];
        uint8_t len        = c[2];

        // Skip duplicates due to burst transmission
        if (send_count == latest_send_count) {
            unit.flush();
            return;
        }
        latest_send_count = send_count;

        M5.Log.printf("RECEIVED: From<%02X> Count:%u Len:%u [%.*s]\n", id, send_count, len, len,
                      (const char*)(c.data() + 3));
        lcd.fillRect(0, 10, lcd.width(), lcd.height() - 10, TFT_DARKCYAN);
        lcd.setCursor(0, 10);
        lcd.setTextSize(1);
        lcd.printf("%.*s", len, (const char*)(c.data() + 3));
        unit.flush();
        M5.Speaker.tone(2000, 20);
    }
}

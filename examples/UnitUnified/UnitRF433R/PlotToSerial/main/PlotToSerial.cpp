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
#include <wiring/m5_unit_unified_wiring.hpp>  // board-aware connection helpers (include last)

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

#if defined(M5_UNIT_UNIFIED_HAS_RMT) && !M5_UNIT_UNIFIED_HAS_RMT
    // UnitRF433 requires the RMT peripheral
    M5_LOGE("RMT is not supported on this target");
    m5::unit::wiring::failStop();
#endif

    // UnitRF433R: RX (input) only, PortB preferred, fallback to PortA
    if (!m5::unit::wiring::addGPIO(Units, unit, m5::unit::wiring::GpioRole::InOnly) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
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

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS   = 2000;
    constexpr TickType_t FEED_SLEEP_TICKS = pdMS_TO_TICKS(5);
    static uint32_t s_next_feed_ms        = 0;
    const uint32_t now_ms                 = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    if (now_ms >= s_next_feed_ms) {
        s_next_feed_ms = now_ms + FEED_INTERVAL_MS;
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feedIdleTaskPeriodically();
#endif
        loop();
    }
}
#endif

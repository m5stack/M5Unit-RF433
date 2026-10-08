/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitRF433T
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedRF433.h>
#include <esp_random.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // board-aware connection helpers (include last)

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitRF433T unit;

// Payload size sweep test.
// Use this to find the maximum reliable payload size for your receiver environment.
// Edit sweep_sizes[] to narrow down the boundary for your setup.
// The receiver's max depends on RMT hardware and RF noise (see rf433::MaxPayloadSize).
constexpr uint8_t sweep_sizes[] = {16, 20, 21, 22, 23, 24, 25, 32, 64, 128, 255};
constexpr size_t SWEEP_COUNT    = sizeof(sweep_sizes) / sizeof(sweep_sizes[0]);
uint8_t sweep_buf[255];
uint8_t sweep_idx{};

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

    // UnitRF433T: TX (output) only, PortB preferred, fallback to PortA
    if (!m5::unit::wiring::addGPIO(Units, unit, m5::unit::wiring::GpioRole::OutOnly) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }

    auto* custom = static_cast<m5::unit::rf433::M5Codec*>(&unit.codec());
    custom->setCommunicationIdentifier(esp_random());

    // Fill sweep buffer with printable pattern
    for (uint16_t i = 0; i < 255; ++i) {
        sweep_buf[i] = 'A' + (i % 26);
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());
    M5.Log.printf("MyID: %02X\n", custom->communicationIdentifier());
    lcd.fillScreen(TFT_DARKGREEN);
    lcd.setCursor(0, 0);
    lcd.setTextSize(1);
    lcd.printf("TX %02X", custom->communicationIdentifier());
}

void loop()
{
    M5.update();
    Units.update();  // Send in update()

    // Send: payload size sweep
    if (M5.BtnA.wasClicked()) {
        uint8_t sz        = sweep_sizes[sweep_idx];
        sweep_buf[sz - 1] = '\0';  // null terminate
        M5.Log.printf("Send: %u bytes\n", sz);
        lcd.fillScreen(TFT_BLUE);
        unit.push_back(sweep_buf, sz);
        unit.send();
        lcd.fillScreen(TFT_DARKGREEN);
        lcd.setCursor(0, 0);
        lcd.setTextSize(1);
        lcd.printf("TX %02X\n%u bytes",
                   static_cast<m5::unit::rf433::M5Codec*>(&unit.codec())->communicationIdentifier(), sz);
        sweep_buf[sz - 1] = 'A' + ((sz - 1) % 26);  // restore pattern

        if (++sweep_idx >= SWEEP_COUNT) {
            sweep_idx = 0;
        }
        M5.Speaker.tone(4000, 20);
    }
}

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS{2000};
    constexpr TickType_t FEED_SLEEP_TICKS{pdMS_TO_TICKS(5)};
    static uint32_t s_last_feed_ms{};
    const uint32_t now_ms{static_cast<uint32_t>(esp_timer_get_time() / 1000)};
    if (now_ms - s_last_feed_ms >= FEED_INTERVAL_MS) {
        s_last_feed_ms = now_ms;
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

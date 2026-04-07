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

    auto* custom = static_cast<m5::unit::rf433::M5Codec*>(unit.codec().get());
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
                   static_cast<m5::unit::rf433::M5Codec*>(unit.codec().get())->communicationIdentifier(), sz);
        sweep_buf[sz - 1] = 'A' + ((sz - 1) % 26);  // restore pattern

        if (++sweep_idx >= SWEEP_COUNT) {
            sweep_idx = 0;
        }
        M5.Speaker.tone(4000, 20);
    }
}

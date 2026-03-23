/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file Transceiver.ino
  @brief UnitRF433T/R example
  NOTICE: Devices to be connected must have multiple ports
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedRF433.h>
#include <esp_random.h>

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitRF433T transmitter;
m5::unit::UnitRF433R receiver;

m5::unit::rf433::communication_identifier_t my_id{};

// Payload size sweep test.
// Use this to find the maximum reliable payload size for your receiver environment.
constexpr uint8_t sweep_sizes[] = {16, 20, 21, 22, 23, 24, 25, 32, 64, 128, 255};
constexpr size_t SWEEP_COUNT    = sizeof(sweep_sizes) / sizeof(sweep_sizes[0]);
uint8_t sweep_buf[255];
uint8_t sweep_idx{};

uint8_t latest_send_count{0xFF};
int16_t half_h{};  // LCD half height

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    auto port_a_in  = M5.getPin(m5::pin_name_t::port_a_pin1);
    auto port_a_out = M5.getPin(m5::pin_name_t::port_a_pin2);
    auto port_b_in  = M5.getPin(m5::pin_name_t::port_b_in);
    auto port_b_out = M5.getPin(m5::pin_name_t::port_b_out);
    M5_LOGI("PortA:%d,%d PortB:%d,%d", port_a_in, port_a_out, port_b_in, port_b_out);

    if (M5.getBoard() == m5::board_t::board_ArduinoNessoN1 || port_a_in < 0 || port_a_out < 0 || port_b_in < 0 ||
        port_b_out < 0 || port_a_in == port_b_in || port_a_out == port_b_out) {
        M5_LOGE("Not enough port");
        if (M5.getBoard() == m5::board_t::board_ArduinoNessoN1) {
            M5_LOGE("NessoN1: PortA is internal I2C (IOExpander), cannot use as GPIO");
        }
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    Wire.end();
    if (!Units.add(transmitter, port_a_in, port_a_out) ||  // PortA: UnitRF433T
        !Units.add(receiver, port_b_in, port_b_out) ||     // PortB: UnitRF433R
        !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    // TAG specification by ESP_DRAM_LOGx does not work, so use wildcards
    esp_log_level_set("*", ESP_LOG_NONE);  // Disable RMT warning log

    //
    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
    my_id = esp_random();
    static_cast<m5::unit::rf433::M5Codec*>(transmitter.codec().get())->setCommunicationIdentifier(my_id);
    M5.Log.printf("MyID: %02X\n", my_id);

    // Fill sweep buffer with printable pattern
    for (uint16_t i = 0; i < 255; ++i) {
        sweep_buf[i] = 'A' + (i % 26);
    }

    // Upper half: TX (green), Lower half: RX (cyan)
    half_h = lcd.height() / 2;
    lcd.fillRect(0, 0, lcd.width(), half_h, TFT_DARKGREEN);
    lcd.fillRect(0, half_h, lcd.width(), half_h, TFT_DARKCYAN);
    lcd.setCursor(0, 0);
    lcd.setTextSize(1);
    lcd.printf("TX %02X", my_id);
    lcd.setCursor(0, half_h);
    lcd.print("RX");
}

void loop()
{
    M5.update();
    Units.update();

    // Receive
    if (receiver.updated()) {
        const auto& c = receiver.container();
        // Container format: ID(1) + Count(1) + Length(1) + Payload(n) (M5Codec)
        if (c.size() < 3) {
            receiver.flush();
            return;
        }

        uint8_t id         = c[0];
        uint8_t send_count = c[1];
        uint8_t len        = c[2];

#if 0
        // Skip if self message
        if (id == my_id) {
            M5_LOGW("Skip message from me");
            receiver.flush();
            return;
        }
#endif

        // Skip duplicates due to burst transmission
        if (send_count == latest_send_count) {
            receiver.flush();
            return;
        }
        latest_send_count = send_count;

        M5.Log.printf("RECEIVED: From<%02X> Count:%u Len:%u [%.*s]\n", id, send_count, len, len,
                      (const char*)(c.data() + 3));
        lcd.fillRect(0, half_h + 10, lcd.width(), half_h - 10, TFT_DARKCYAN);
        lcd.setCursor(0, half_h + 10);
        lcd.setTextSize(1);
        lcd.printf("%.*s", len, (const char*)(c.data() + 3));
        receiver.flush();
        M5.Speaker.tone(2000, 20);
    }

    // Send: payload size sweep
    if (M5.BtnA.wasClicked()) {
        uint8_t sz        = sweep_sizes[sweep_idx];
        sweep_buf[sz - 1] = '\0';  // null terminate
        M5.Log.printf("Send: %u bytes\n", sz);
        lcd.fillRect(0, 0, lcd.width(), half_h, TFT_BLUE);
        transmitter.push_back(sweep_buf, sz);
        transmitter.send();
        lcd.fillRect(0, 0, lcd.width(), half_h, TFT_DARKGREEN);
        lcd.setCursor(0, 0);
        lcd.setTextSize(1);
        lcd.printf("TX %02X\n%u bytes", my_id, sz);
        sweep_buf[sz - 1] = 'A' + ((sz - 1) % 26);  // restore pattern

        if (++sweep_idx >= SWEEP_COUNT) {
            sweep_idx = 0;
        }
        M5.Speaker.tone(4000, 20);
    }
}

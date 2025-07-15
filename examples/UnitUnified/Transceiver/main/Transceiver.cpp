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
const char* msg[4] = {"Beam Me Up!", "Live Long and Prosper", "Engage!", "Make it so"};
// const char* msg[4] = {"Beam", "Live", "Engage", "Make"};

uint8_t msg_index{};

uint8_t latest_send_count{0xFF};

}  // namespace

void setup()
{
    M5.begin();

    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    auto port_a_in  = M5.getPin(m5::pin_name_t::port_a_pin1);
    auto port_a_out = M5.getPin(m5::pin_name_t::port_a_pin2);
    auto port_b_in  = M5.getPin(m5::pin_name_t::port_b_in);
    auto port_b_out = M5.getPin(m5::pin_name_t::port_b_out);
    // auto port_c_in  = M5.getPin(m5::pin_name_t::port_c_in);
    // auto port_c_out = M5.getPin(m5::pin_name_t::port_c_out);
    M5_LOGI("A:%d,%d B:%d,%d", port_a_in, port_a_out, port_b_in, port_b_out);

    if (port_a_in < 0 || port_a_out < 0 || port_b_in < 0 || port_b_out < 0) {
        M5_LOGE("Not enough port");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    Wire.end();
    if (!Units.add(transmitter, port_a_in, port_a_out) ||  // PortA: UnitRF433T
        !Units.add(receiver, port_b_in, port_b_out) ||     // PortB: UnitRF433R
        !Units.begin()) {
        M5_LOGE("Failed to begin");
        lcd.clear(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    // TAG specification by ESP_DRAM_LOGx does not work, so use wildcards
    esp_log_level_set("*", ESP_LOG_NONE);  // Disable RMT warning log

    //
    M5_LOGI("M5UnitUnified has been begun");
    M5_LOGI("%s", Units.debugInfo().c_str());
    lcd.fillScreen(TFT_DARKGREEN);

    my_id = esp_random();
    transmitter.setCommunicationIdentifier(my_id);
    M5.Log.printf("MyID;%X", my_id);
}

void loop()
{
    using namespace m5::unit::rf433;

    M5.update();
    auto touch = M5.Touch.getDetail();
    Units.update();

    // Receive
    if (receiver.updated()) {
        const auto& c = receiver.container();
        // m5::utility::log::dump(c.data(), c.size(), false);

        auto prot = c[0];  // front is protocol
        uint32_t id{};
        uint8_t send_count = latest_send_count;
        uint32_t offset{1};

        if (prot & ProtocolIncludeIdentifier) {
            id = *(uint32_t*)(c.data() + offset);
            offset += 4;
#if 0
            // Skip if self message
            if (id == my_id) {
                M5_LOGW("Skip message from me");
                receiver.flush();
                return;
            }
#endif
        }
        if (prot & ProtocolIncludeSendCount) {
            send_count = c[offset++];
            // Skip duplicates due to burst transmission
            if (send_count == latest_send_count) {
                receiver.flush();
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
        receiver.flush();
    }

    // Send
    if (M5.BtnA.wasClicked() || touch.wasClicked()) {
        auto ptr = msg[msg_index++];
        msg_index &= 3;
        M5.Log.printf("Send:[%s] %zu bytes\n", ptr, strlen(ptr) + 1);
        transmitter.push_back((uint8_t*)ptr, strlen(ptr) + 1 /*include '\0' */);
        // Send in update()
        M5.Speaker.tone(4000, 20);
    }
}

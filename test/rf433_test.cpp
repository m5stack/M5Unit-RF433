/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Common tests for RF433 encode/decode and CRC (native/embedded)
*/
#include <gtest/gtest.h>
#include <unit/rmt_item_types.hpp>
#include <m5_utility/crc.hpp>
#include <cstring>

using namespace m5::unit::rf433;
using namespace m5::unit::gpio;

// ============================================================
// Manchester Encode
// ============================================================

TEST(ManchesterEncode, SingleByte0xFF)
{
    uint8_t data = 0xFF;
    auto result  = encodeManchester(&data, 1);
    ASSERT_EQ(result.size(), 8u);
    for (const auto& item : result) {
        EXPECT_EQ(item.duration0, 800u);
        EXPECT_EQ(item.duration1, 200u);
    }
}

TEST(ManchesterEncode, SingleByte0x00)
{
    uint8_t data = 0x00;
    auto result  = encodeManchester(&data, 1);
    ASSERT_EQ(result.size(), 8u);
    for (const auto& item : result) {
        EXPECT_EQ(item.duration0, 200u);
        EXPECT_EQ(item.duration1, 800u);
    }
}

TEST(ManchesterEncode, MultiByte)
{
    uint8_t data[] = {0xAB, 0xCD};
    auto result    = encodeManchester(data, 2);
    EXPECT_EQ(result.size(), 16u);
}

TEST(ManchesterEncode, EmptyInput)
{
    auto result = encodeManchester(nullptr, 0);
    EXPECT_TRUE(result.empty());
}

TEST(ManchesterEncode, MSBOrder)
{
    // 0x80 = 10000000 MSB first: bit7=1 then 7 zeros
    uint8_t data = 0x80;
    auto result  = encodeManchester(&data, 1, true);
    ASSERT_EQ(result.size(), 8u);
    // First item should be '1' (800, 200)
    EXPECT_EQ(result[0].duration0, 800u);
    EXPECT_EQ(result[0].duration1, 200u);
    // Rest should be '0' (200, 800)
    for (size_t i = 1; i < 8; ++i) {
        EXPECT_EQ(result[i].duration0, 200u);
        EXPECT_EQ(result[i].duration1, 800u);
    }
}

TEST(ManchesterEncode, LSBOrder)
{
    // 0x80 = 10000000 LSB first: bit0=0 first, bit7=1 last
    uint8_t data = 0x80;
    auto result  = encodeManchester(&data, 1, false);
    ASSERT_EQ(result.size(), 8u);
    // First 7 items should be '0' (200, 800)
    for (size_t i = 0; i < 7; ++i) {
        EXPECT_EQ(result[i].duration0, 200u);
        EXPECT_EQ(result[i].duration1, 800u);
    }
    // Last item should be '1' (800, 200)
    EXPECT_EQ(result[7].duration0, 800u);
    EXPECT_EQ(result[7].duration1, 200u);
}

TEST(ManchesterEncode, Deterministic)
{
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    auto r1        = encodeManchester(data, sizeof(data));
    auto r2        = encodeManchester(data, sizeof(data));
    ASSERT_EQ(r1.size(), r2.size());
    EXPECT_EQ(std::memcmp(r1.data(), r2.data(), r1.size() * sizeof(m5_rmt_item_t)), 0);
}

TEST(ManchesterEncode, MaxPayload)
{
    uint8_t data[MaxPayloadSize]{};
    auto result = encodeManchester(data, MaxPayloadSize);
    EXPECT_EQ(result.size(), (size_t)MaxPayloadSize * 8);
}

// ============================================================
// CRC8
// ============================================================

TEST(CRC8, Deterministic)
{
    m5::utility::CRC8_Checksum crc1, crc2;
    uint8_t data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F};
    crc1.update(data, sizeof(data));
    crc2.update(data, sizeof(data));
    EXPECT_EQ(crc1.value(), crc2.value());
}

TEST(CRC8, StreamingEquivalent)
{
    m5::utility::CRC8_Checksum crc1, crc2;
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    crc1.update(data, 4);
    crc2.update(data, 2);
    crc2.update(data + 2, 2);
    EXPECT_EQ(crc1.value(), crc2.value());
}

TEST(CRC8, ClearResets)
{
    m5::utility::CRC8_Checksum crc;
    uint8_t data[] = {0x42};
    crc.update(data, 1);
    uint8_t v1 = crc.value();
    crc.clear();
    crc.update(data, 1);
    EXPECT_EQ(crc.value(), v1);
}

TEST(CRC8, DifferentDataDifferentCRC)
{
    m5::utility::CRC8_Checksum crc1, crc2;
    uint8_t d1[] = {0x00};
    uint8_t d2[] = {0x01};
    crc1.update(d1, 1);
    crc2.update(d2, 1);
    EXPECT_NE(crc1.value(), crc2.value());
}

// ============================================================
// Protocol Constants
// ============================================================

TEST(Protocol, Overhead)
{
    // CRC8(1) + ID(1) + Count(1) + Length(1) = 4
    EXPECT_EQ(ProtocolOverhead, 4);
}

// ============================================================
// calculateRingBufferSize
// ============================================================

TEST(RingBufferSize, SmallPayload)
{
    // 1 byte payload
    // SOF(1) + (overhead(4)+payload(1))*8 = 1+40 = 41 items
    // 41 * 4 + 2 = 166
    EXPECT_EQ(calculateRingBufferSize(1), 166u);
}

TEST(RingBufferSize, TenBytes)
{
    // SOF(1) + (4+10)*8 = 1+112 = 113 items
    // 113 * 4 + 2 = 454
    EXPECT_EQ(calculateRingBufferSize(10), 454u);
}

TEST(RingBufferSize, ZeroPayload)
{
    // SOF(1) + (4+0)*8 = 1+32 = 33 items
    // 33 * 4 + 2 = 134
    EXPECT_EQ(calculateRingBufferSize(0), 134u);
}

TEST(PayloadLimit, MaxPayloadSize)
{
    // MaxPayloadSize must be > 0 and <= 255
    EXPECT_GT(MaxPayloadSize, 0);
    EXPECT_LE(MaxPayloadSize, 255);
    // Ring buffer for max payload must fit in uint16_t
    EXPECT_LE(calculateRingBufferSize(MaxPayloadSize), 0xFFFFu);
}

// ============================================================
// Manchester Decode
// ============================================================
// decodeManchester reads bit boundaries BETWEEN consecutive RMT items:
//   d.duration0 = items[i-1].duration1  (low duration of previous item)
//   d.duration1 = items[i].duration0    (high duration of current item)
//   bit = (d.duration0 > d.duration1) ? 1 : 0
//   total must be in (600, 1100)
//
// To represent a '1' bit: prev.duration1=800, curr.duration0=200 (800>200)
// To represent a '0' bit: prev.duration1=200, curr.duration0=800 (200<800)

// Helper: build RMT items that decode to the given byte (MSB order)
static std::vector<m5_rmt_item_t> buildDecodeItems(const uint8_t* data, size_t len)
{
    std::vector<m5_rmt_item_t> items;
    // First item: duration0 is irrelevant, duration1 sets up the first bit boundary
    bool first_bit = (data[0] >> 7) & 1;
    m5_rmt_item_t first{};
    first.level0    = 1;
    first.level1    = 0;
    first.duration0 = 500;                    // arbitrary
    first.duration1 = first_bit ? 800 : 200;  // sets up first bit's d.duration0
    items.push_back(first);

    for (size_t b = 0; b < len; ++b) {
        for (int bit = 0; bit < 8; ++bit) {
            bool is_one      = (data[b] >> (7 - bit)) & 1;
            bool next_is_one = false;
            // Determine the next bit for setting duration1
            int next_bit_idx = bit + 1;
            if (next_bit_idx < 8) {
                next_is_one = (data[b] >> (7 - next_bit_idx)) & 1;
            } else if (b + 1 < len) {
                next_is_one = (data[b + 1] >> 7) & 1;
            }

            m5_rmt_item_t item{};
            item.level0    = 1;
            item.level1    = 0;
            item.duration0 = is_one ? 200 : 800;
            // Last bit of last byte: duration1 doesn't matter
            bool is_last   = (b == len - 1 && bit == 7);
            item.duration1 = is_last ? 500 : (next_is_one ? 800 : 200);
            items.push_back(item);
        }
    }
    return items;
}

TEST(ManchesterDecode, NullBuffer)
{
    m5_rmt_item_t items[2]{};
    EXPECT_EQ(decodeManchester(nullptr, 10, items, 2), 0);
}

TEST(ManchesterDecode, ZeroSize)
{
    m5_rmt_item_t items[2]{};
    uint8_t buf[1]{};
    EXPECT_EQ(decodeManchester(buf, 0, items, 2), 0);
}

TEST(ManchesterDecode, NullItems)
{
    uint8_t buf[1]{};
    EXPECT_EQ(decodeManchester(buf, 1, nullptr, 2), 0);
}

TEST(ManchesterDecode, TooFewItems)
{
    m5_rmt_item_t items[1]{};
    uint8_t buf[1]{};
    EXPECT_EQ(decodeManchester(buf, 1, items, 1), 0);
}

TEST(ManchesterDecode, SingleByte0xFF)
{
    uint8_t input = 0xFF;
    auto items    = buildDecodeItems(&input, 1);
    ASSERT_EQ(items.size(), 9u);  // 1 setup + 8 data

    uint8_t buf[4]{};
    uint16_t decoded = decodeManchester(buf, sizeof(buf), items.data(), items.size());
    EXPECT_EQ(decoded, 1u);
    EXPECT_EQ(buf[0], 0xFF);
}

TEST(ManchesterDecode, SingleByte0x00)
{
    uint8_t input = 0x00;
    auto items    = buildDecodeItems(&input, 1);
    ASSERT_EQ(items.size(), 9u);

    uint8_t buf[4]{};
    uint16_t decoded = decodeManchester(buf, sizeof(buf), items.data(), items.size());
    EXPECT_EQ(decoded, 1u);
    EXPECT_EQ(buf[0], 0x00);
}

TEST(ManchesterDecode, SingleByte0xA5)
{
    uint8_t input = 0xA5;  // 10100101
    auto items    = buildDecodeItems(&input, 1);

    uint8_t buf[4]{};
    uint16_t decoded = decodeManchester(buf, sizeof(buf), items.data(), items.size());
    EXPECT_EQ(decoded, 1u);
    EXPECT_EQ(buf[0], 0xA5);
}

TEST(ManchesterDecode, MultiByte)
{
    uint8_t input[] = {0xDE, 0xAD};
    auto items      = buildDecodeItems(input, 2);
    ASSERT_EQ(items.size(), 17u);  // 1 setup + 16 data

    uint8_t buf[4]{};
    uint16_t decoded = decodeManchester(buf, sizeof(buf), items.data(), items.size());
    EXPECT_EQ(decoded, 2u);
    EXPECT_EQ(buf[0], 0xDE);
    EXPECT_EQ(buf[1], 0xAD);
}

TEST(ManchesterDecode, BufferTooSmall)
{
    // 2-byte input but only 1-byte output buffer (plus guard for partial)
    // buf_size is checked as idx < buf_size - 1, so buf_size=2 allows idx 0 only
    uint8_t input[] = {0xAB, 0xCD};
    auto items      = buildDecodeItems(input, 2);

    uint8_t buf[2]{};
    uint16_t decoded = decodeManchester(buf, 2, items.data(), items.size());
    // Should decode at most 1 full byte (idx < buf_size-1 = 1)
    EXPECT_EQ(decoded, 1u);
    EXPECT_EQ(buf[0], 0xAB);
}

TEST(ManchesterDecode, InvalidTiming)
{
    // Items with total duration outside valid range should cause early exit
    m5_rmt_item_t items[3] = {
        {{500, 1, 100, 0}},  // setup: duration1=100
        {{100, 1, 500, 0}},  // d0=100, d1=100, total=200 < 600 → invalid
        {{500, 1, 500, 0}},  // should not be reached
    };

    uint8_t buf[4]{};
    uint16_t decoded = decodeManchester(buf, sizeof(buf), items, 3);
    EXPECT_EQ(decoded, 0u);
}

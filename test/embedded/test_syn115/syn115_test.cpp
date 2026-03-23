/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for SYN115
*/
#include <gtest/gtest.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <unit/unit_SYN115.hpp>
#include <cstring>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::rf433;

class TestSYN115 : public GPIOComponentTestBase<UnitSYN115> {
protected:
    virtual UnitSYN115* get_instance() override
    {
        auto ptr = new m5::unit::UnitSYN115();
        if (ptr) {
            auto cfg           = ptr->config();
            cfg.send_in_update = false;  // Prevent auto-send during update()
            ptr->config(cfg);
        }
        return ptr;
    }
};

TEST_F(TestSYN115, Basic)
{
    SCOPED_TRACE(ustr);
}

TEST_F(TestSYN115, Config)
{
    SCOPED_TRACE(ustr);
    auto cfg                     = unit->config();
    cfg.burst_transmission_count = 8;
    cfg.send_in_update           = false;
    unit->config(cfg);

    auto cfg2 = unit->config();
    EXPECT_EQ(cfg2.burst_transmission_count, 8);
    EXPECT_FALSE(cfg2.send_in_update);
}

TEST_F(TestSYN115, CommunicationIdentifier)
{
    SCOPED_TRACE(ustr);
    auto* custom = static_cast<rf433::M5Codec*>(unit->codec().get());
    custom->setCommunicationIdentifier(0xAB);
    EXPECT_EQ(custom->communicationIdentifier(), 0xAB);

    custom->setCommunicationIdentifier(0x00);
    EXPECT_EQ(custom->communicationIdentifier(), 0x00);
}

TEST_F(TestSYN115, PushBack)
{
    SCOPED_TRACE(ustr);
    uint8_t data[] = {0x01, 0x02, 0x03};
    EXPECT_TRUE(unit->push_back(data, sizeof(data)));
}

TEST_F(TestSYN115, PushBackLimit)
{
    SCOPED_TRACE(ustr);
    uint8_t data[255]{};
    memset(data, 0xAA, sizeof(data));

    // Exactly 255 bytes: should succeed
    EXPECT_TRUE(unit->push_back(data, 255));
    unit->clear();

    // 256 bytes: should fail
    uint8_t data256[256]{};
    EXPECT_FALSE(unit->push_back(data256, 256));
}

TEST_F(TestSYN115, PushBackOverflow)
{
    SCOPED_TRACE(ustr);
    uint8_t data[200]{};
    memset(data, 0xBB, sizeof(data));
    EXPECT_TRUE(unit->push_back(data, 200));

    // 200 + 56 = 256 > 255: should fail
    uint8_t data2[56]{};
    EXPECT_FALSE(unit->push_back(data2, 56));

    // 200 + 55 = 255: should succeed
    unit->clear();
    EXPECT_TRUE(unit->push_back(data, 200));
    uint8_t data3[55]{};
    EXPECT_TRUE(unit->push_back(data3, 55));
}

TEST_F(TestSYN115, Clear)
{
    SCOPED_TRACE(ustr);
    uint8_t data[] = {0x01, 0x02, 0x03};
    EXPECT_TRUE(unit->push_back(data, sizeof(data)));

    unit->clear();

    // After clear, should be able to push_back full payload again
    uint8_t data2[255]{};
    EXPECT_TRUE(unit->push_back(data2, 255));
}

TEST_F(TestSYN115, SendEmpty)
{
    SCOPED_TRACE(ustr);
    // send() without push_back should return false
    EXPECT_FALSE(unit->send());
}

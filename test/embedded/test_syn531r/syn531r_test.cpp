/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for SYN531R
*/
#include <gtest/gtest.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <unit/unit_SYN531R.hpp>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::rf433;

class TestSYN531R : public GPIOComponentTestBase<UnitSYN531R> {
protected:
    virtual UnitSYN531R* get_instance() override
    {
        auto ptr = new m5::unit::UnitSYN531R();
        return ptr;
    }
};

TEST_F(TestSYN531R, Basic)
{
    SCOPED_TRACE(ustr);
}

TEST_F(TestSYN531R, InitialState)
{
    SCOPED_TRACE(ustr);
    EXPECT_TRUE(unit->empty());
    EXPECT_EQ(unit->available(), 0u);
    EXPECT_EQ(unit->oldest(), 0);
    EXPECT_EQ(unit->latest(), 0);
}

TEST_F(TestSYN531R, FlushOnEmpty)
{
    SCOPED_TRACE(ustr);
    unit->flush();
    EXPECT_TRUE(unit->empty());
}

TEST_F(TestSYN531R, DiscardOnEmpty)
{
    SCOPED_TRACE(ustr);
    unit->discard();
    EXPECT_TRUE(unit->empty());
}

TEST_F(TestSYN531R, Config)
{
    SCOPED_TRACE(ustr);
    auto cfg             = unit->config();
    cfg.max_payload_size = 10;
    unit->config(cfg);
    auto cfg2 = unit->config();
    EXPECT_EQ(cfg2.max_payload_size, 10);
}

TEST_F(TestSYN531R, MaxPayloadSize)
{
    SCOPED_TRACE(ustr);
    auto cfg = unit->config();

    // Default should be <= MaxPayloadSize
    EXPECT_LE(cfg.max_payload_size, rf433::MaxPayloadSize);
    EXPECT_GT(cfg.max_payload_size, 0);

    // Setting a custom value should be retained
    cfg.max_payload_size = 10;
    unit->config(cfg);
    EXPECT_EQ(unit->config().max_payload_size, 10);
}

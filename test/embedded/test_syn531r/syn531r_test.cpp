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
#include <utility>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::rf433;

// Unit component tests (requires RMT)
#if !defined(M5_UNIT_UNIFIED_HAS_RMT) || M5_UNIT_UNIFIED_HAS_RMT
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
#endif

// Codec pointer must follow the moved-to unit (no hardware required)
TEST(SYN531RCodec, Move)
{
    // Default codec: moved-to unit must refer to its own default codec
    {
        UnitSYN531R src;
        static_cast<M5Codec*>(&src.codec())->setCommunicationIdentifier(0x5A);

        UnitSYN531R dst(std::move(src));
        EXPECT_EQ(&dst.codec(), &static_cast<const UnitSYN531R&>(dst).codec());
        EXPECT_NE(&dst.codec(), &src.codec());
        EXPECT_EQ(static_cast<M5Codec*>(&dst.codec())->communicationIdentifier(), 0x5A);

        UnitSYN531R assigned;
        auto* old_codec = &assigned.codec();
        assigned        = std::move(dst);
        EXPECT_EQ(&assigned.codec(), old_codec);
        EXPECT_NE(&assigned.codec(), &dst.codec());
        EXPECT_EQ(static_cast<M5Codec*>(&assigned.codec())->communicationIdentifier(), 0x5A);
    }

    // External codec: moved-to unit keeps referring to it
    {
        M5Codec external{};
        UnitSYN531R src;
        src.setCodec(external);
        EXPECT_EQ(&src.codec(), &external);

        UnitSYN531R dst(std::move(src));
        EXPECT_EQ(&dst.codec(), &external);

        // Reset returns to the unit's own default codec
        dst.resetCodec();
        EXPECT_NE(&dst.codec(), &external);
        EXPECT_NE(&dst.codec(), &src.codec());
    }

    // Move-assign onto a unit using an external codec: it returns to its own default codec
    {
        M5Codec external{};
        UnitSYN531R src;
        UnitSYN531R assigned;
        auto* own_codec = &assigned.codec();
        assigned.setCodec(external);
        EXPECT_EQ(&assigned.codec(), &external);

        assigned = std::move(src);
        EXPECT_EQ(&assigned.codec(), own_codec);
        EXPECT_NE(&assigned.codec(), &external);
        EXPECT_NE(&assigned.codec(), &src.codec());
    }

    // setCodec() with the unit's own built-in codec stays move-safe
    {
        UnitSYN531R src;
        src.setCodec(src.codec());
        UnitSYN531R dst(std::move(src));
        EXPECT_NE(&dst.codec(), &src.codec());
    }
}

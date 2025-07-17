/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for SYN115
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <unit/unit_SYN115.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::rf433;
using namespace m5::unit::types;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestSYN115 : public GPIOComponentTestBase<UnitSYN115, bool> {
protected:
    virtual UnitSYN115* get_instance() override
    {
        auto ptr = new m5::unit::UnitSYN115();
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSYN115, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSYN115,::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestSYN115, ::testing::Values(false));

namespace {
}  // namespace

TEST_P(TestSYN115, Basic)
{
    SCOPED_TRACE(ustr);
}

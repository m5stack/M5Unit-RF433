/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for SYN531R
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <unit/unit_SYN531R.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::rf433;
using namespace m5::unit::types;

const ::testing::Environment* global_fixture = ::testing::AddGlobalTestEnvironment(new GlobalFixture<400000U>());

class TestSYN531R : public GPIOComponentTestBase<UnitSYN531R, bool> {
protected:
    virtual UnitSYN531R* get_instance() override
    {
        auto ptr = new m5::unit::UnitSYN531R();
        return ptr;
    }
    virtual bool is_using_hal() const override
    {
        return GetParam();
    };
};

// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSYN531R, ::testing::Values(false, true));
// INSTANTIATE_TEST_SUITE_P(ParamValues, TestSYN531R,::testing::Values(true));
INSTANTIATE_TEST_SUITE_P(ParamValues, TestSYN531R, ::testing::Values(false));

namespace {
}  // namespace

TEST_P(TestSYN531R, Basic)
{
    SCOPED_TRACE(ustr);
}

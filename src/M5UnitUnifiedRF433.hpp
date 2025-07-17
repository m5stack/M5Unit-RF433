/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*!
  @file M5UnitUnifiedRF433.hpp
  @brief Main header of M5Unit-RF433 using M5UnitUnified

  @mainpage M5Unit-RF433
  Library for UnitRF433 using M5UnitUnified.
*/
#ifndef M5_UNIT_UNIFIED_RF433_HPP
#define M5_UNIT_UNIFIED_RF433_HPP

#include "unit/unit_SYN115.hpp"
#include "unit/unit_SYN531R.hpp"

/*!
  @namespace m5
  @brief Top level namespace of M5stack
 */
namespace m5 {

/*!
  @namespace unit
  @brief Unit-related namespace
 */
namespace unit {

using UnitRF433T = UnitSYN115;
using UnitRF433R = UnitSYN531R;

}  // namespace unit
}  // namespace m5
#endif

# ESP-IDF component manifest for M5Unit-RF433.
# Upstream: https://github.com/m5stack/M5Unit-RF433
#
# UnitRF433T (SYN115, RMT TX) and UnitRF433R (SYN531R, RMT RX). The public headers include
# M5UnitUnified (M5UnitComponent.hpp) and M5Utility (m5_utility/crc.hpp from codec/m5_codec.hpp), so
# both are public requirements. The RMT channels are driven through M5UnitUnified's GPIO adapter.
file(GLOB_RECURSE SRCS "src/*.cpp")
idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS "src"
    REQUIRES M5UnitUnified M5Utility
)

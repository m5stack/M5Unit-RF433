# Map the Kconfig "Target transmitter" / "Target receiver" choices (common/Kconfig.variant.*) to the
# source-level macros that both the Arduino and ESP-IDF builds use. Include this from an example's
# main/CMakeLists.txt *after* idf_component_register() (it needs ${COMPONENT_LIB}).
# The transmitter and receiver choices are independent (Transceiver uses both), so each one can add
# its own macro.
set(M5UNIT_VARIANTS "")
# Transmitter (common/Kconfig.variant.tx). UnitRF433T (GROVE) is the source default and needs no macro.
# Add `elseif(CONFIG_EXAMPLE_USING_<X>) list(APPEND M5UNIT_VARIANTS USING_<X>)` for a new transmitter.
if(CONFIG_EXAMPLE_USING_UNIT_RF433T)
endif()
# Receiver (common/Kconfig.variant.rx). UnitRF433R (GROVE) is the source default and needs no macro.
# Add `elseif(CONFIG_EXAMPLE_USING_<X>) list(APPEND M5UNIT_VARIANTS USING_<X>)` for a new receiver.
if(CONFIG_EXAMPLE_USING_UNIT_RF433R)
endif()
if(M5UNIT_VARIANTS)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ${M5UNIT_VARIANTS})
endif()

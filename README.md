# M5Unit - RF433

## Overview

### SKU:U113

RF433R is an RF wireless radio frequency receiver, using SYN531R radio frequency receiver, working frequency is 433.92MHz (using ASK modulation) commonly used by wireless controllers, built-in PCB antenna, stable receiving signal distance up to 10M, typical receiving sensitivity It is -109dBm. The exquisite and compact housing design can be embedded in a variety of radio frequency remote control applications. It is suitable for security alarm, wireless automatic meter reading, home and industrial automation, remote control, wireless data transmission and other system fields.


### SKU:U114

RF433T is a radio frequency (RF) transmitter, using SYN115 radio frequency transmitter IC, working frequency is 433.92MHz (using ASK modulation) commonly used by wireless controllers, built-in PCB antenna, stable signal transmission distance up to 10M, The output power reaches 10dBm. The exquisite and compact housing design can be embedded in a variety of radio frequency remote control applications. It is suitable for security alarm, wireless automatic meter reading, home and industrial automation, remote control, wireless data transmission and other system fields.


## CAUTION

Please follow the radio laws in the location where it is used.


## Related Link

- [Unit RF433R - Document & Datasheet](https://docs.m5stack.com/en/unit/rf433_r)
- [Unit RF433T - Document & Datasheet](https://docs.m5stack.com/en/unit/rf433_t)

## Required Libraries:

- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)


## License

- [M5Unit-RF433 - MIT](LICENSE)


## Support via [PbHub](https://docs.m5stack.com/en/unit/pbhub_1.1)

|Unit|Support|Note|
|---|---|---|
|UnitRF433T|NG|RMT (precise pulse timing) not supported by PbHub|
|UnitRF433R|NG|RMT (precise pulse timing) not supported by PbHub|


## Payload size
RF433 (433.92 MHz ASK) is intended for short messages (a few to a few tens of bytes).
Each frame is Manchester-encoded and protected by CRC8 automatically.

The receivable payload size depends on the **receiver** side:

| Receiver environment | RMT | Default `max_payload_size` | Theoretical max |
|---|---|---|---|
| ESP32 / ESP32-S2 / ESP32-S3 / ESP32-C3 with Arduino 2.x (ESP-IDF 4.x) | v1 | 23 | 43 (ESP32-S2: 27) |
| Arduino 3.x / ESP-IDF 5.x or later | v2 | 255 | 255 |

- Keep the transmitted payload within the receiver's `max_payload_size`, especially when the transmitter is RMT v2 and the receiver is RMT v1.
- For longer data, split it into packets (e.g. 20-30 bytes each with a sequence number) and reassemble on the receiver.
- At longer distances or in noisy environments, a long frame is more likely to be lost (a single bit error discards the whole frame) or, rarely, to pass the CRC8 check while corrupted. Prefer short packets, repeat them with `burst_transmission_count`, and add your own integrity check (e.g. CRC16) for important data.

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

### For ESP-IDF settings

> **NOTE:** The ESP-IDF native build (`idf.py`) targets ESP-IDF **5.1 or later** (5.x and 6.x) on esp32 / esp32s3 / esp32c3 / esp32c5 / esp32c6 / esp32h2 / esp32p4. ESP32-C2 / C61 are not supported (no RMT).

On ESP-IDF native builds (`idf.py`), the unit is selected via Kconfig instead of editing the source. Each example exposes the choice through `main/Kconfig.projbuild`, which sources the Kconfig files in `examples/UnitUnified/common/`:

| Kconfig file | Variants offered | Used by |
|---|---|---|
| `Kconfig.variant.tx` | UnitRF433T (U114) | UnitRF433T/PlotToSerial, Transceiver |
| `Kconfig.variant.rx` | UnitRF433R (U113) | UnitRF433R/PlotToSerial, Transceiver |

`examples/UnitUnified/common/variant.cmake` maps the chosen `CONFIG_EXAMPLE_USING_*` to the source-level macro shared with the Arduino build. Each choice currently has a single option (the default), so no `menuconfig` step is needed:

```sh
cd examples/UnitUnified/UnitRF433R/PlotToSerial    # or UnitRF433T/PlotToSerial, Transceiver
idf.py set-target esp32s3                          # or esp32 / esp32c6 / esp32h2 / ...
idf.py build flash monitor
```

## Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-RF433/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

### Required
- [Doxygen](https://www.doxygen.nl/)
- [Git](https://git-scm.com/) (Output commit hash to html)

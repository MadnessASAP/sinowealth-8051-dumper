# Flash memory dumper for 8051-based SinoWealth MCUs
Beware, this project is rather POC than user-friendly tool. I successfully used it to dump firmware from SH68F881W MCU in Genesis Thor 300 keyboard, but it should work with other chips of the same type.

## Hardware Requirements
- Arduino Uno R3 (or compatible ATmega328P-based board)
- Target SinoWealth MCU with accessible JTAG pins

## How to build

This project uses PlatformIO for building and uploading.

### Prerequisites
- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) or [PlatformIO IDE](https://platformio.org/install/ide)

### Building
```bash
# Build the project
pio run

# Upload to Arduino Uno
pio run --target upload

# Open serial monitor (115200 baud)
pio device monitor
```

### Configuration
Before building, check the chip configuration in `include/config.h` and update it if needed. The parameters can be retrieved from Keil C51 definition files (*.opt, *.gpt) inside UV4 folder.

## How to use

### Pin Connections
Connect the following Arduino Uno pins to the corresponding JTAG pins of the SinoWealth MCU:
- D2 (TDO) - JTAG TDO
- D3 (TMS) - JTAG TMS
- D4 (TDI) - JTAG TDI
- D5 (TCK) - JTAG TCK
- D6 (VREF) - Target MCU power supply (for voltage reference detection)
- GND - Ground

### Power Sequence
The dumper now includes VREF detection to prevent powering the target via I/O leakage:
1. Power up the Arduino Uno
2. The dumper will wait for VREF (D6) to go high
3. Manually enable power to the target MCU
4. Once VREF is detected, the dumper will proceed with the connection sequence

The host has to communicate with target MCU within few tens of milliseconds since powering up. Therefore, if you use one of Arduino boards, I recommend you to change bootloader to Optiboot to get rid of bootloader delay. If it's not enough, next thing to try is to set SUT fuses of ATmega MCU to 4.1ms pre-delay (65ms is the default).

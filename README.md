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
Connect D2-D5 pins and power rail of your Arduino Uno to corresponding JTAG pins of SinoWealth MCU and power the host up. You should immediately observe messages on the serial port (115200 baud).

The host has to communicate with target MCU within few tens of milliseconds since powering up. Therefore, if you use one of Arduino boards, I recommend you to change bootloader to Optiboot to get rid of bootloader delay. If it's not enough, next thing to try is to set SUT fuses of ATmega MCU to 4.1ms pre-delay (65ms is the default).

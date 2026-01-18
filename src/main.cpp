/*
   https://github.com/gashtaan/sinowealth-8051-dumper

   Copyright (C) 2023, Michal Kovacik

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include <simpleRPC.h>
#include "jtag.h"
#include "config.h"

// Global JTAG instance
JTAG* jtag = nullptr;
uint8_t buffer[256] = {};  // Increased buffer size for larger reads

// RPC Functions

/**
 * Connect to the target device via JTAG
 * Returns true if connection was successful
 */
bool rpc_connect() {
    if (!jtag) {
        jtag = new JTAG();
    }
    jtag->connect();
    return true;
}

/**
 * Disconnect from the target device
 */
void rpc_disconnect() {
    if (jtag) {
        jtag->disconnect();
    }
}

/**
 * Check if ICP mode is working
 * Returns true if ICP communication is successful
 */
bool rpc_checkICP() {
    if (!jtag) {
        return false;
    }
    return jtag->checkICP();
}

/**
 * Check if JTAG mode is working
 * Returns true if JTAG communication is successful
 */
bool rpc_checkJTAG() {
    if (!jtag) {
        return false;
    }
    return jtag->checkJTAG();
}

/**
 * Get the JTAG ID code from the device
 * Returns the 16-bit ID code
 */
unsigned int rpc_getID() {
    if (!jtag) {
        return 0;
    }
    return jtag->getID();
}

/**
 * Send a ping to the device in ICP mode
 */
void rpc_pingICP() {
    if (jtag) {
        jtag->pingICP();
    }
}

/**
 * Read a single byte from flash using ICP mode
 * Returns the byte value or 0xFF on error
 */
unsigned char rpc_readByteICP(unsigned long address, bool customBlock) {
    if (!jtag) {
        return 0xFF;
    }

    uint8_t byte = 0xFF;
    if (jtag->readFlashICP(&byte, 1, address, customBlock)) {
        return byte;
    }
    return 0xFF;
}

/**
 * Read a single byte from flash using JTAG mode
 * Returns the byte value or 0xFF on error
 */
unsigned char rpc_readByteJTAG(unsigned long address, bool customBlock) {
    if (!jtag) {
        return 0xFF;
    }

    uint8_t byte = 0xFF;
    if (jtag->readFlashJTAG(&byte, 1, address, customBlock)) {
        return byte;
    }
    return 0xFF;
}

/**
 * Read 16 bytes from flash using ICP mode
 * Stores data in global buffer and returns success status
 */
bool rpc_read16ICP(unsigned long address, bool customBlock) {
    if (!jtag) {
        return false;
    }
    return jtag->readFlashICP(buffer, 16, address, customBlock);
}

/**
 * Read 16 bytes from flash using JTAG mode
 * Stores data in global buffer and returns success status
 */
bool rpc_read16JTAG(unsigned long address, bool customBlock) {
    if (!jtag) {
        return false;
    }
    return jtag->readFlashJTAG(buffer, 16, address, customBlock);
}

/**
 * Get byte from buffer at index
 */
unsigned char rpc_getBufferByte(unsigned char index) {
    if (index < sizeof(buffer)) {
        return buffer[index];
    }
    return 0xFF;
}

/**
 * Auto-detect the best flash read method
 * Returns 0 for failure, 1 for ICP, 2 for JTAG
 */
unsigned char rpc_detectReadMethod() {
    if (!jtag) {
        return 0;
    }

    // Try ICP method first
    if (jtag->readFlashICP(buffer, 4, 0, false)) {
        uint32_t* data = (uint32_t*)buffer;
        if (*data != 0) {
            return 1;  // ICP works
        }
    }

    // Try JTAG method
    if (jtag->readFlashJTAG(buffer, 4, 0, false)) {
        uint32_t* data = (uint32_t*)buffer;
        if (*data != 0) {
            return 2;  // JTAG works
        }
    }

    return 0;  // Neither method works or flash is blank
}

/**
 * Get the product block address based on chip configuration
 * Returns the address or 0 if not applicable
 */
unsigned int rpc_getProductBlockAddress() {
    switch (CHIP_CUSTOM_BLOCK) {
        case 2:
            return 0x0A00;
        case 3:
            return 0x1200;
        case 4:
            return 0x2200;
        default:
            return 0;
    }
}

/**
 * Get the code options address based on chip configuration
 */
unsigned int rpc_getCodeOptionsAddress() {
    uint16_t options_size = 64;
    uint16_t options_address = CHIP_FLASH_SIZE - options_size;

    switch (CHIP_CUSTOM_BLOCK) {
        case 2:
            if (CHIP_TYPE == 2)
                options_address = 0x0800;
            break;
        case 3:
            if (CHIP_TYPE == 2)
                options_address = 0x1000;
            else if (CHIP_TYPE == 7)
                options_address = 0x1000;
            break;
        case 4:
            options_address = 0x2000;
            break;
        case 6:
            options_size = 32;
            options_address = CHIP_FLASH_SIZE - options_size;
            break;
    }

    return options_address;
}

/**
 * Get the code options size based on chip configuration
 */
unsigned int rpc_getCodeOptionsSize() {
    uint16_t options_size = 64;

    switch (CHIP_CUSTOM_BLOCK) {
        case 3:
            if (CHIP_TYPE == 7)
                options_size = 512;
            break;
        case 6:
            options_size = 32;
            break;
    }

    return options_size;
}

/**
 * Check if code options are in flash or custom block
 */
bool rpc_getCodeOptionsInFlash() {
    bool options_in_flash = true;

    switch (CHIP_CUSTOM_BLOCK) {
        case 2:
            if (CHIP_TYPE == 2)
                options_in_flash = false;
            break;
        case 3:
            if (CHIP_TYPE == 2 || CHIP_TYPE == 7)
                options_in_flash = false;
            break;
        case 4:
            options_in_flash = false;
            break;
    }

    return options_in_flash;
}

/**
 * Get chip type from configuration
 */
unsigned char rpc_getChipType() {
    return CHIP_TYPE;
}

/**
 * Get flash size from configuration
 */
unsigned long rpc_getFlashSize() {
    return CHIP_FLASH_SIZE;
}

/**
 * Get product block flag from configuration
 */
unsigned char rpc_getProductBlock() {
    return CHIP_PRODUCT_BLOCK;
}

/**
 * Get custom block type from configuration
 */
unsigned char rpc_getCustomBlock() {
    return CHIP_CUSTOM_BLOCK;
}

// Arduino setup function
void setup() {
    // Initialize serial using Arduino's Serial object for SimpleRPC compatibility
    Serial.begin(115200);
    Serial.println(F("\r\nSinoWealth 8051-based MCU flash dumper (RPC mode)"));
    Serial.println(F("Ready for commands"));
}

// Arduino loop function - handles RPC calls
void loop() {
    // SimpleRPC interface - automatically handles RPC calls
    interface(
        rpc_connect, "connect", "Connect to target device: @return: Success status.",
        rpc_disconnect, "disconnect", "Disconnect from target device.",
        rpc_checkICP, "checkICP", "Check if ICP mode is working: @return: True if ICP communication is successful.",
        rpc_checkJTAG, "checkJTAG", "Check if JTAG mode is working: @return: True if JTAG communication is successful.",
        rpc_getID, "getID", "Get JTAG ID code: @return: 16-bit ID code.",
        rpc_pingICP, "pingICP", "Send ping to device in ICP mode.",
        rpc_readByteICP, "readByteICP", "Read single byte via ICP: @address: Address. @customBlock: Custom block flag. @return: Byte value.",
        rpc_readByteJTAG, "readByteJTAG", "Read single byte via JTAG: @address: Address. @customBlock: Custom block flag. @return: Byte value.",
        rpc_read16ICP, "read16ICP", "Read 16 bytes via ICP to buffer: @address: Address. @customBlock: Custom block flag. @return: Success status.",
        rpc_read16JTAG, "read16JTAG", "Read 16 bytes via JTAG to buffer: @address: Address. @customBlock: Custom block flag. @return: Success status.",
        rpc_getBufferByte, "getBufferByte", "Get byte from buffer: @index: Buffer index. @return: Byte value.",
        rpc_detectReadMethod, "detectReadMethod", "Auto-detect best read method: @return: 0=failed, 1=ICP, 2=JTAG.",
        rpc_getProductBlockAddress, "getProductBlockAddress", "Get product block address: @return: Address or 0.",
        rpc_getCodeOptionsAddress, "getCodeOptionsAddress", "Get code options address: @return: Address.",
        rpc_getCodeOptionsSize, "getCodeOptionsSize", "Get code options size: @return: Size in bytes.",
        rpc_getCodeOptionsInFlash, "getCodeOptionsInFlash", "Check if options in flash: @return: True if in flash.",
        rpc_getChipType, "getChipType", "Get chip type: @return: Chip type.",
        rpc_getFlashSize, "getFlashSize", "Get flash size: @return: Flash size in bytes.",
        rpc_getProductBlock, "getProductBlock", "Get product block flag: @return: Product block flag.",
        rpc_getCustomBlock, "getCustomBlock", "Get custom block type: @return: Custom block type."
    );
}

// Main function for non-Arduino builds
int main() {
    init();  // Arduino initialization
    setup();
    while (true) {
        loop();
    }
    return 0;
}

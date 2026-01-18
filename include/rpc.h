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

#pragma once

/**
 * Initialize RPC system
 */
void rpc_init();

/**
 * Process RPC commands - should be called from Arduino loop()
 */
void rpc_loop();

// RPC function declarations

/**
 * Connect to the target device via JTAG
 * Returns true if connection was successful
 */
bool rpc_connect();

/**
 * Disconnect from the target device
 */
void rpc_disconnect();

/**
 * Check if ICP mode is working
 * Returns true if ICP communication is successful
 */
bool rpc_checkICP();

/**
 * Check if JTAG mode is working
 * Returns true if JTAG communication is successful
 */
bool rpc_checkJTAG();

/**
 * Get the JTAG ID code from the device
 * Returns the 16-bit ID code
 */
unsigned int rpc_getID();

/**
 * Send a ping to the device in ICP mode
 */
void rpc_pingICP();

/**
 * Read a single byte from flash using ICP mode
 * Returns the byte value or 0xFF on error
 */
unsigned char rpc_readByteICP(unsigned long address, bool customBlock);

/**
 * Read a single byte from flash using JTAG mode
 * Returns the byte value or 0xFF on error
 */
unsigned char rpc_readByteJTAG(unsigned long address, bool customBlock);

/**
 * Read 16 bytes from flash using ICP mode
 * Stores data in global buffer and returns success status
 */
bool rpc_read16ICP(unsigned long address, bool customBlock);

/**
 * Read 16 bytes from flash using JTAG mode
 * Stores data in global buffer and returns success status
 */
bool rpc_read16JTAG(unsigned long address, bool customBlock);

/**
 * Get byte from buffer at index
 */
unsigned char rpc_getBufferByte(unsigned char index);

/**
 * Auto-detect the best flash read method
 * Returns 0 for failure, 1 for ICP, 2 for JTAG
 */
unsigned char rpc_detectReadMethod();

/**
 * Get the product block address based on chip configuration
 * Returns the address or 0 if not applicable
 */
unsigned int rpc_getProductBlockAddress();

/**
 * Get the code options address based on chip configuration
 */
unsigned int rpc_getCodeOptionsAddress();

/**
 * Get the code options size based on chip configuration
 */
unsigned int rpc_getCodeOptionsSize();

/**
 * Check if code options are in flash or custom block
 */
bool rpc_getCodeOptionsInFlash();

/**
 * Get chip type from configuration
 */
unsigned char rpc_getChipType();

/**
 * Get flash size from configuration
 */
unsigned long rpc_getFlashSize();

/**
 * Get product block flag from configuration
 */
unsigned char rpc_getProductBlock();

/**
 * Get custom block type from configuration
 */
unsigned char rpc_getCustomBlock();

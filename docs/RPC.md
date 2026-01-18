# RPC Interface

The firmware exposes the following RPC functions via the [SimpleRPC](https://simplerpc.readthedocs.io/) library over serial at 115200 baud.

## Connection Management

### `connect()`
Connect to the target device via JTAG.

**Returns**: `bool` - True if connection was successful

---

### `disconnect()`
Disconnect from the target device.

**Returns**: `void`

---

### `checkICP()`
Check if ICP (In-Circuit Programming) mode communication is working.

**Returns**: `bool` - True if ICP communication is successful

---

### `checkJTAG()`
Check if JTAG mode communication is working.

**Returns**: `bool` - True if JTAG communication is successful

---

### `pingICP()`
Send a ping command to the device in ICP mode.

**Returns**: `void`

---

## Device Information

### `getID()`
Get the JTAG ID code from the target device.

**Returns**: `unsigned int` - 16-bit JTAG ID code

---

### `detectReadMethod()`
Auto-detect the best flash read method for the target device.

**Returns**: `unsigned char`
- `0` - Detection failed (flash blank or protected)
- `1` - ICP mode works
- `2` - JTAG mode works

---

### `getChipType()`
Get the configured chip type from `config.h`.

**Returns**: `unsigned char` - Chip type

---

### `getFlashSize()`
Get the total flash memory size from `config.h`.

**Returns**: `unsigned long` - Flash size in bytes

---

### `getProductBlock()`
Get the product block configuration flag from `config.h`.

**Returns**: `unsigned char` - Product block flag (0 or 1)

---

### `getCustomBlock()`
Get the custom block type configuration from `config.h`.

**Returns**: `unsigned char` - Custom block type (0-6)

---

### `getProductBlockAddress()`
Get the memory address of the product block based on chip configuration.

**Returns**: `unsigned int` - Product block address (0 if not applicable)

---

### `getCodeOptionsAddress()`
Get the memory address of code options based on chip configuration.

**Returns**: `unsigned int` - Code options address

---

### `getCodeOptionsSize()`
Get the size of the code options area based on chip configuration.

**Returns**: `unsigned int` - Size in bytes

---

### `getCodeOptionsInFlash()`
Check if code options are stored in flash memory or custom block.

**Returns**: `bool` - True if in flash, False if in custom block

---

## Flash Reading

### `readByteICP(address, customBlock)`
Read a single byte from flash using ICP mode.

**Parameters**:
- `address` (`unsigned long`) - Memory address to read from
- `customBlock` (`bool`) - True to read from custom block area

**Returns**: `unsigned char` - Byte value (0xFF on error)

---

### `readByteJTAG(address, customBlock)`
Read a single byte from flash using JTAG mode.

**Parameters**:
- `address` (`unsigned long`) - Memory address to read from
- `customBlock` (`bool`) - True to read from custom block area

**Returns**: `unsigned char` - Byte value (0xFF on error)

---

### `read16ICP(address, customBlock)`
Read 16 bytes from flash using ICP mode into internal buffer.

**Parameters**:
- `address` (`unsigned long`) - Starting memory address
- `customBlock` (`bool`) - True to read from custom block area

**Returns**: `bool` - True if read was successful

**Note**: Use `getBufferByte(index)` to retrieve individual bytes after calling this function.

---

### `read16JTAG(address, customBlock)`
Read 16 bytes from flash using JTAG mode into internal buffer.

**Parameters**:
- `address` (`unsigned long`) - Starting memory address
- `customBlock` (`bool`) - True to read from custom block area

**Returns**: `bool` - True if read was successful

**Note**: Use `getBufferByte(index)` to retrieve individual bytes after calling this function.

---

### `getBufferByte(index)`
Get a byte from the internal buffer (populated by `read16ICP` or `read16JTAG`).

**Parameters**:
- `index` (`unsigned char`) - Buffer index (0-15)

**Returns**: `unsigned char` - Byte value (0xFF if index out of bounds)

---

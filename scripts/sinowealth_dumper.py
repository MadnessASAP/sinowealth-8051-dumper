#!/usr/bin/env python3
"""
SinoWealth 8051 Flash Dumper - Python RPC Client

Communicates with the Arduino-based flash dumper via SimpleRPC to read
flash memory from SinoWealth 8051-based MCUs.

Requires: pip install simple-rpc

Copyright (C) 2024
License: GPL-3.0
"""

import argparse
import sys
import time
from collections.abc import Callable
from pathlib import Path
from typing import Any

from simple_rpc import Interface  # pyright: ignore[reportMissingTypeStubs]

# simple_rpc.Interface is dynamically typed - methods are generated at runtime from RPC
# Type alias to make this explicit
RPCInterface = Any


class DebugRPCWrapper:
    """Wrapper that logs all RPC calls and responses."""

    def __init__(self, interface: RPCInterface) -> None:
        self._interface: RPCInterface = interface

    def __getattr__(self, name: str) -> Any:
        attr = getattr(self._interface, name)
        if callable(attr):

            def wrapper(*args: Any, **kwargs: Any) -> Any:
                args_str = ", ".join(
                    [repr(a) for a in args] + [f"{k}={v!r}" for k, v in kwargs.items()]
                )
                print(f"[RPC] {name}({args_str})", end="", flush=True)
                try:
                    result = attr(*args, **kwargs)
                    print(f" -> {result!r}")
                    return result
                except Exception as e:
                    print(f" -> ERROR: {e}")
                    raise

            return wrapper
        return attr

    def close(self) -> None:
        """Pass through close method."""
        if hasattr(self._interface, "close"):
            self._interface.close()


class ReadMethod:
    """Flash read method constants."""

    FAILED: int = 0
    ICP: int = 1
    JTAG: int = 2
    AUTO: int = 3


CHIP_TYPES: dict[int, str] = {
    0: "Unknown",
    1: "Type 1 (64KB max)",
    2: "Type 2 (64KB max)",
    3: "Type 3 (64KB max)",
    4: "Type 4 (1MB max)",
    5: "Type 5 (64KB max)",
    6: "Type 6 (64KB max)",
    7: "Type 7 (128KB max)",
}


class SinoWealthDumper:
    """Interface for SinoWealth 8051 flash dumper."""

    def __init__(
        self, port: str, baudrate: int = 115200, debug_rpc: bool = False
    ) -> None:
        """
        Initialize connection to the Arduino dumper.

        Args:
            port: Serial port (e.g., /dev/ttyUSB0, COM3)
            baudrate: Serial baud rate (default 115200)
            debug_rpc: Print all RPC calls and responses
        """
        self.port: str = port
        self.baudrate: int = baudrate
        self.debug_rpc: bool = debug_rpc
        self.interface: RPCInterface | DebugRPCWrapper | None = None
        self._connected: bool = False

    def open(self) -> bool:
        """Open serial connection to the Arduino."""
        try:
            interface = Interface(self.port, self.baudrate)  # pyright: ignore[reportArgumentType]
            if self.debug_rpc:
                self.interface = DebugRPCWrapper(interface)
            else:
                self.interface = interface
            return True
        except Exception as e:
            print(f"Error opening serial port: {e}")
            return False

    def close(self) -> None:
        """Close the serial connection."""
        if self.interface:
            try:
                self.interface.close()
            except Exception:
                pass
            self.interface = None

    def connect(self) -> bool:
        """
        Connect to the target device via JTAG.

        Returns:
            True if connection was successful
        """
        if not self.interface:
            return False
        print("Power cycle or reset the target now...")
        result = self.interface.connect()
        self._connected = result
        return result

    def disconnect(self) -> None:
        """Disconnect from the target device."""
        if self.interface:
            self.interface.disconnect()
            self._connected = False

    def check_icp(self) -> bool:
        """Check if ICP mode communication is working."""
        if not self.interface:
            return False
        return self.interface.checkICP()

    def check_jtag(self) -> bool:
        """Check if JTAG mode communication is working."""
        if not self.interface:
            return False
        return self.interface.checkJTAG()

    def get_id(self) -> int:
        """Get the JTAG ID code from the target device."""
        if not self.interface:
            return 0
        return self.interface.getID()

    def detect_read_method(self) -> int:
        """
        Auto-detect the best flash read method.

        Returns:
            0 - Detection failed
            1 - ICP mode works
            2 - JTAG mode works
        """
        if not self.interface:
            return ReadMethod.FAILED
        return self.interface.detectReadMethod()

    def get_chip_type(self) -> int:
        """Get the configured chip type."""
        if not self.interface:
            return 0
        return self.interface.getChipType()

    def get_flash_size(self) -> int:
        """Get the configured flash size in bytes."""
        if not self.interface:
            return 0
        return self.interface.getFlashSize()

    def get_product_block(self) -> int:
        """Get the product block configuration flag."""
        if not self.interface:
            return 0
        return self.interface.getProductBlock()

    def get_custom_block(self) -> int:
        """Get the custom block type configuration."""
        if not self.interface:
            return 0
        return self.interface.getCustomBlock()

    def get_product_block_address(self) -> int:
        """Get the memory address of the product block."""
        if not self.interface:
            return 0
        return self.interface.getProductBlockAddress()

    def get_code_options_address(self) -> int:
        """Get the memory address of code options."""
        if not self.interface:
            return 0
        return self.interface.getCodeOptionsAddress()

    def get_code_options_size(self) -> int:
        """Get the size of the code options area."""
        if not self.interface:
            return 0
        return self.interface.getCodeOptionsSize()

    def get_code_options_in_flash(self) -> bool:
        """Check if code options are stored in flash memory."""
        if not self.interface:
            return False
        return self.interface.getCodeOptionsInFlash()

    def read_byte_icp(self, address: int, custom_block: bool = False) -> int:
        """Read a single byte using ICP mode."""
        if not self.interface:
            return 0xFF
        return self.interface.readByteICP(address, custom_block)

    def read_byte_jtag(self, address: int, custom_block: bool = False) -> int:
        """Read a single byte using JTAG mode."""
        if not self.interface:
            return 0xFF
        return self.interface.readByteJTAG(address, custom_block)

    def read_16_icp(self, address: int, custom_block: bool = False) -> bool:
        """Read 16 bytes using ICP mode into internal buffer."""
        if not self.interface:
            return False
        return self.interface.read16ICP(address, custom_block)

    def read_16_jtag(self, address: int, custom_block: bool = False) -> bool:
        """Read 16 bytes using JTAG mode into internal buffer."""
        if not self.interface:
            return False
        return self.interface.read16JTAG(address, custom_block)

    def get_buffer_byte(self, index: int) -> int:
        """Get a byte from the internal buffer (0-15)."""
        if not self.interface or index < 0 or index > 15:
            return 0xFF
        return self.interface.getBufferByte(index)

    def get_buffer(self) -> bytes:
        """Get all 16 bytes from the internal buffer."""
        return bytes(self.get_buffer_byte(i) for i in range(16))

    def read_flash(
        self,
        start_address: int = 0,
        length: int | None = None,
        method: int = ReadMethod.AUTO,
        custom_block: bool = False,
        progress_callback: Callable[[int, int], None] | None = None,
    ) -> bytes:
        """
        Read flash memory from the target.

        Args:
            start_address: Starting address (default 0)
            length: Number of bytes to read (default: full flash)
            method: Read method (AUTO, ICP, or JTAG)
            custom_block: Read from custom block area
            progress_callback: Optional callback(current, total) for progress

        Returns:
            Bytes read from flash
        """
        if length is None:
            length = self.get_flash_size() - start_address

        # Auto-detect method if needed
        if method == ReadMethod.AUTO:
            detected = self.detect_read_method()
            if detected == ReadMethod.FAILED:
                print("Warning: Auto-detection failed, trying ICP mode")
                method = ReadMethod.ICP
            else:
                method = detected
                method_name = "ICP" if method == ReadMethod.ICP else "JTAG"
                print(f"Auto-detected read method: {method_name}")

        # Select read function
        if method == ReadMethod.ICP:
            read_16 = self.read_16_icp
        else:
            read_16 = self.read_16_jtag

        # Align start address to 16-byte boundary for efficiency
        aligned_start = start_address & ~0xF
        skip_bytes = start_address - aligned_start
        total_to_read = length + skip_bytes
        aligned_length = (total_to_read + 15) & ~0xF

        data = bytearray()
        address = aligned_start
        end_address = aligned_start + aligned_length

        while address < end_address:
            if not read_16(address, custom_block):
                print(f"\nError reading at address 0x{address:06X}")
                break

            data.extend(self.get_buffer())

            if progress_callback:
                progress_callback(len(data) - skip_bytes, length)

            address += 16

        # Trim to requested range
        return bytes(data[skip_bytes : skip_bytes + length])


def print_device_info(dumper: SinoWealthDumper) -> None:
    """Print target device information."""
    print("\n=== Device Information ===")

    jtag_id = dumper.get_id()
    print(f"JTAG ID:          0x{jtag_id:04X}")

    chip_type = dumper.get_chip_type()
    chip_desc = CHIP_TYPES.get(chip_type, f"Unknown ({chip_type})")
    print(f"Chip Type:        {chip_desc}")

    flash_size = dumper.get_flash_size()
    print(f"Flash Size:       {flash_size} bytes ({flash_size // 1024} KB)")

    product_block = dumper.get_product_block()
    print(f"Product Block:    {'Enabled' if product_block else 'Disabled'}")

    if product_block:
        pb_addr = dumper.get_product_block_address()
        print(f"  Address:        0x{pb_addr:04X}")

    custom_block = dumper.get_custom_block()
    print(f"Custom Block:     Type {custom_block}")

    co_addr = dumper.get_code_options_address()
    co_size = dumper.get_code_options_size()
    co_in_flash = dumper.get_code_options_in_flash()
    print(f"Code Options:     0x{co_addr:04X} ({co_size} bytes)")
    print(f"  Location:       {'Flash' if co_in_flash else 'Custom Block'}")

    # Check communication modes
    print("\n=== Communication Status ===")
    icp_ok = dumper.check_icp()
    jtag_ok = dumper.check_jtag()
    print(f"ICP Mode:         {'OK' if icp_ok else 'Failed'}")
    print(f"JTAG Mode:        {'OK' if jtag_ok else 'Failed'}")

    detected = dumper.detect_read_method()
    if detected == ReadMethod.ICP:
        print("Recommended:      ICP")
    elif detected == ReadMethod.JTAG:
        print("Recommended:      JTAG")
    else:
        print("Recommended:      Detection failed (flash may be blank or protected)")

    print()


def progress_bar(current: int, total: int, width: int = 50) -> None:
    """Display a progress bar."""
    percent = current / total if total > 0 else 0
    filled = int(width * percent)
    bar = "█" * filled + "░" * (width - filled)
    print(
        f"\r[{bar}] {percent * 100:5.1f}% ({current}/{total} bytes)", end="", flush=True
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="SinoWealth 8051 Flash Dumper - Python RPC Client",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s -p /dev/ttyUSB0 --info
  %(prog)s -p /dev/ttyUSB0 -o firmware.bin
  %(prog)s -p /dev/ttyUSB0 -o firmware.bin --method icp
  %(prog)s -p /dev/ttyUSB0 -o partial.bin --start 0x1000 --length 4096
        """,
    )

    parser.add_argument(
        "-p",
        "--port",
        required=True,
        help="Serial port (e.g., /dev/ttyUSB0, /dev/ttyACM0, COM3)",
    )
    parser.add_argument(
        "-b",
        "--baudrate",
        type=int,
        default=115200,
        help="Serial baud rate (default: 115200)",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="Output file for flash dump",
    )
    parser.add_argument(
        "--info",
        action="store_true",
        help="Display device information only",
    )
    parser.add_argument(
        "--method",
        choices=["auto", "icp", "jtag"],
        default="auto",
        help="Flash read method (default: auto)",
    )
    parser.add_argument(
        "--start",
        type=lambda x: int(x, 0),
        default=0,
        help="Start address (default: 0)",
    )
    parser.add_argument(
        "--length",
        type=lambda x: int(x, 0),
        default=None,
        help="Number of bytes to read (default: full flash)",
    )
    parser.add_argument(
        "--custom-block",
        action="store_true",
        help="Read from custom block area instead of main flash",
    )
    parser.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="Suppress progress output",
    )
    parser.add_argument(
        "--debug-rpc",
        action="store_true",
        help="Print all RPC calls and responses",
    )

    args = parser.parse_args()

    # Map method string to constant
    method_map = {
        "auto": ReadMethod.AUTO,
        "icp": ReadMethod.ICP,
        "jtag": ReadMethod.JTAG,
    }
    method = method_map[args.method]

    # Create dumper instance
    dumper = SinoWealthDumper(args.port, args.baudrate, debug_rpc=args.debug_rpc)

    print(f"Opening serial port {args.port}...")
    if not dumper.open():
        sys.exit(1)

    try:
        print("Connecting to target...")
        if not dumper.connect():
            print("Error: Failed to connect to target device.")
            print("Make sure the target is powered and connected correctly.")
            sys.exit(1)

        print("Connected successfully!")

        # Always show basic info
        if args.info or not args.output:
            print_device_info(dumper)

        # Dump flash if output specified
        if args.output:
            flash_size = dumper.get_flash_size()
            length = (
                args.length if args.length is not None else (flash_size - args.start)
            )

            print(f"Reading {length} bytes from address 0x{args.start:06X}...")
            print(f"Method: {args.method.upper()}")

            callback = None if args.quiet else progress_bar

            start_time = time.time()
            data = dumper.read_flash(
                start_address=args.start,
                length=length,
                method=method,
                custom_block=args.custom_block,
                progress_callback=callback,
            )
            elapsed = time.time() - start_time

            if not args.quiet:
                print()  # Newline after progress bar

            if len(data) == length:
                args.output.write_bytes(data)
                speed = len(data) / elapsed if elapsed > 0 else 0
                print(f"Saved {len(data)} bytes to {args.output}")
                print(f"Transfer speed: {speed:.1f} bytes/sec")
            else:
                print(f"Warning: Only read {len(data)} of {length} bytes")
                if len(data) > 0:
                    args.output.write_bytes(data)
                    print(f"Partial dump saved to {args.output}")
                sys.exit(1)

        if not args.info and not args.output:
            print("No action specified. Use --info or --output.")
            print("Run with --help for usage information.")

    finally:
        dumper.disconnect()
        dumper.close()


if __name__ == "__main__":
    main()

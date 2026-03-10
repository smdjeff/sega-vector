#!/usr/bin/env python3

'''
Packs LPC files into 2732 ROMs with an index table as a header

'''

import argparse
import os
import struct
import sys

# Constants
CHUNK_SIZE = 4096
MAX_SIZE = 4 * CHUNK_SIZE # 4x 2732 ROMs
INDEX_ZERO_VAL = 0x0000

def pack_lpc_files(input_dir, output_prefix, lpc_order=12):
    frame_size = 3 + lpc_order
    # 1. Gather and Sort .lpc files
    lpc_files = sorted([f for f in os.listdir(input_dir) if f.endswith('.lpc')])
    if not lpc_files:
        print(f"No .lpc files found in {input_dir}")
        return

    # 2. Read Data and store sizes
    file_data_blocks = []
    file_sizes = []
    for filename in lpc_files:
        with open(os.path.join(input_dir, filename), 'rb') as f:
            data = f.read()
            if len(data) % frame_size != 0:
                print(f"WARNING: {filename} size {len(data)} is not a multiple of "
                      f"frame size {frame_size} (order {lpc_order})", file=sys.stderr)
            file_data_blocks.append(data)
            file_sizes.append(len(data))

    # 3. Header Calculation (16-bit offsets)
    # Index 0 = 0x0000, then one 16-bit offset per file
    header_size = (1 + len(lpc_files)) * 2
    offsets = [INDEX_ZERO_VAL]
    current_ptr = header_size
    combined_data = b""

    for data in file_data_blocks:
        offsets.append(current_ptr)
        combined_data += data
        current_ptr += len(data)

    # 4. Construct Binary & Pad with 0xFF
    header_binary = b"".join(struct.pack('<H', off) for off in offsets)
    full_package = bytearray(header_binary + combined_data)

    remainder = len(full_package) % CHUNK_SIZE
    if remainder > 0:
        padding_size = CHUNK_SIZE - remainder
        full_package.extend(b'\xff' * padding_size)

    # 5. Safety Check
    if len(full_package) > MAX_SIZE:
        raise ValueError(f"ERROR: size ({len(full_package)}) exceeds MAX_SIZE ({MAX_SIZE})")

    # 6. Diagnostic Table (Updated with Size and ROM)
    print(f"\nLPC order: {lpc_order}  frame size: {frame_size} bytes")
    print(f"\n{'Index':<7} | {'Offset':<10} | {'Size (B)':<10} | {'Frames':<8} | {'ROM(s)':<10} | {'Filename'}")
    print("-" * 82)
    # Print the Null Index
    print(f"[{0:02}]     | {hex(offsets[0]):<10} | {'-':<10} | {'-':<8} | {'-':<10} | Reserved/Null")
    
    # Print the Files
    for i in range(1, len(offsets)):
        idx = i
        off = offsets[i]
        size = file_sizes[i-1]
        name = lpc_files[i-1]
        frames = size // frame_size
        rom_start = off // CHUNK_SIZE + 1
        rom_end = (off + size - 1) // CHUNK_SIZE + 1
        if rom_start == rom_end:
            rom_str = str(rom_start)
        else:
            rom_str = f"{rom_start}-{rom_end}"
        print(f"[{idx:02}]     | {hex(off):<10} | {size:<10} | {frames:<8} | {rom_str:<10} | {name}")
    
    print("-" * 72)
    print(f"Total Package: {len(full_package)} bytes ({len(full_package)//CHUNK_SIZE} ROMs)")
    unpadded_size = len(header_binary) + len(combined_data)
    print(f"Free Space:    {MAX_SIZE - unpadded_size} bytes\n")

    # 7. Split into 4KB Chunks
    for i in range(0, len(full_package), CHUNK_SIZE):
        chunk_index = i // CHUNK_SIZE
        chunk_data = full_package[i : i + CHUNK_SIZE]
        chunk_filename = f"{output_prefix}_{chunk_index}.bin"
        with open(chunk_filename, 'wb') as f:
            f.write(chunk_data)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Pack LPC files into 2732 ROMs with an index table header.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("input_dir", help="Directory containing .lpc files")
    parser.add_argument("output_prefix", help="Output filename prefix for ROM chunks")
    parser.add_argument('--lpc_order', type=int, default=10, choices=[8, 10, 12],
                        help='LPC filter order for frame size validation (8, 10, or 12, default 10)')

    args = parser.parse_args()
    
    try:
        pack_lpc_files(args.input_dir, args.output_prefix, args.lpc_order)
    except Exception as e:
        print(e)
        sys.exit(1)

#!/usr/bin/env python3

'''
Packs LPC files into 2732 ROMs with an index table as a header

'''

import os
import struct
import sys

# Constants
MAX_SIZE = 64 * 1024  # 16KB Total Limit
CHUNK_SIZE = 4096     # 4KB Chunk size
INDEX_ZERO_VAL = 0x0000

def pack_lpc_files(input_dir, output_prefix):
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
        raise ValueError(f"ERROR: Padded size ({len(full_package)}) exceeds MAX_SIZE ({MAX_SIZE})")

    # 6. Diagnostic Table (Updated with Size and ROM)
    print(f"\n{'Index':<7} | {'Offset':<10} | {'Size (B)':<10} | {'ROM(s)':<10} | {'Filename'}")
    print("-" * 72)
    # Print the Null Index
    print(f"[{0:02}]     | {hex(offsets[0]):<10} | {'-':<10} | {'-':<10} | Reserved/Null")
    
    # Print the Files
    for i in range(1, len(offsets)):
        idx = i
        off = offsets[i]
        size = file_sizes[i-1]
        name = lpc_files[i-1]
        rom_start = off // CHUNK_SIZE + 1
        rom_end = (off + size - 1) // CHUNK_SIZE + 1
        if rom_start == rom_end:
            rom_str = str(rom_start)
        else:
            rom_str = f"{rom_start}-{rom_end}"
        print(f"[{idx:02}]     | {hex(off):<10} | {size:<10} | {rom_str:<10} | {name}")
    
    print("-" * 72)
    print(f"Total Package: {len(full_package)} bytes ({len(full_package)//CHUNK_SIZE} ROMs)")
    print(f"Free Space:    {MAX_SIZE - len(full_package)} bytes\n")

    # 7. Split into 4KB Chunks
    for i in range(0, len(full_package), CHUNK_SIZE):
        chunk_index = i // CHUNK_SIZE
        chunk_data = full_package[i : i + CHUNK_SIZE]
        chunk_filename = f"{output_prefix}_{chunk_index}.bin"
        with open(chunk_filename, 'wb') as f:
            f.write(chunk_data)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 pack_lpc.py <input_dir> <output_prefix>")
        sys.exit(1)
    
    try:
        pack_lpc_files(sys.argv[1], sys.argv[2])
    except Exception as e:
        print(e)
        sys.exit(1)

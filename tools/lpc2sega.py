#!/usr/bin/env python3
"""
lpc2rom.py  -  SP0250 LPC binary frames -> Sega G80 8035 ROM bitstream
"""

import sys
from dataclasses import dataclass
from typing import List

@dataclass
class Frame:
    filt_b: List[int]  # 6 bytes
    filt_f: List[int]  # 6 bytes
    amp: int           # 0-255
    pitch: int         # 0-255
    voiced: int = 1    # not used in direct mode


def encode_frames_direct_no_repeat(frames: List[Frame]) -> bytes:
    """
    Encode frames using ONLY MODE 00 full-frame updates.
    Assumes no identical frames and repeat = 1 for every frame.
    """
    output = bytearray()
    prev_pitch = 0

    for frame in frames:
        # MODE 00, repeat=1 -> (repeat-1)=0 -> control=0x00
        control = 0x00
        output.append(control)

        # Amplitude
        output.append(frame.amp & 0xFF)

        # 8-bit pitch delta
        pitch_delta = (frame.pitch - prev_pitch) & 0xFF
        output.append(pitch_delta)
        prev_pitch = frame.pitch

        # Interleaved filter coefficients
        for i in range(6):
            output.append(frame.filt_b[i] & 0xFF)
            output.append(frame.filt_f[i] & 0xFF)

    # End marker
    output.append(0xFF)

    return bytes(output)


def read_lpc_frames(input_path: str) -> List[Frame]:
    """
    Reads the raw LPC binary frames and maps them to Frame objects.
    """
    frames = []
    
    # ASSUMPTION: 15 bytes per frame. Adjust offsets to match your input format.
    FRAME_SIZE = 15 
    
    with open(input_path, 'rb') as f:
        data = f.read()

    for i in range(0, len(data), FRAME_SIZE):
        chunk = data[i:i + FRAME_SIZE]
        
        # Skip an incomplete trailing frame if the file size isn't a perfect multiple
        if len(chunk) < FRAME_SIZE:
            break  

        # Map bytes to their respective Frame attributes
        filt_b = list(chunk[0:6])
        filt_f = list(chunk[6:12])
        amp = chunk[12]
        pitch = chunk[13]
        voiced = chunk[14]

        frames.append(Frame(
            filt_b=filt_b, 
            filt_f=filt_f, 
            amp=amp, 
            pitch=pitch, 
            voiced=voiced
        ))

    return frames


def convert(input_path: str, output_path: str) -> None:
    """
    Orchestrates reading the input binary, encoding the frames, 
    and writing out the new ROM bitstream.
    """
    print(f"Reading frames from: {input_path}")
    frames = read_lpc_frames(input_path)
    
    if not frames:
        print("Warning: No frames were read. Check your input file.")
        sys.exit(1)

    print(f"Successfully loaded {len(frames)} frames.")
    
    rom_data = encode_frames_direct_no_repeat(frames)
    
    with open(output_path, 'wb') as f:
        f.write(rom_data)
        
    print(f"Wrote {len(rom_data)} bytes to: {output_path}")


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: lpc2rom.py input.bin output.bin")
        sys.exit(1)
        
    convert(sys.argv[1], sys.argv[2])
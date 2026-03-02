#!/usr/bin/env python3

'''
Decodes raw SP0250 LPC frames and creates WAV file.

Follows SP0250 Applications Manual for 15 byte stream framing and filter coefficients.
Largely based on MAME's GI SP0250 digital LPC sound synthesizer By O. Galibert.

Note: Currently Sega G80's 8035 decompression scheme is unknown so
      input file must be decompressed SP0250 LPC from. These can be obtainted
      from either a MAME runtime dump or wav2lpc.py
'''

import argparse
import numpy as np
import wave
import sys

class SP0250:
    def __init__(self, sample_rate=9286):
        self.sample_rate = sample_rate
        self.rng = 1
        self.filters = [{"F": 0, "B": 0, "z1": 0.0, "z2": 0.0} for _ in range(6)]
        self.amp = 0
        self.pitch = 0
        self.repeat = 0
        self.voiced = False
        self.pcount = 0
        self.rcount = 0
        
    def _decode_ga(self, v):
        return (v & 0x1f) << (v >> 5)

    def _decode_gc(self, v):
        # SP0250 filter coefficients
        coefs = [
              0,   9,  17,  25,  33,  41,  49,  57,  65,  73,  81,  89,  97, 105, 113, 121,
            129, 137, 145, 153, 161, 169, 177, 185, 193, 201, 209, 217, 225, 233, 241, 249,
            257, 265, 273, 281, 289, 297, 301, 305, 309, 313, 317, 321, 325, 329, 333, 337,
            341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381, 385, 389, 393, 397, 401,
            405, 409, 413, 417, 421, 425, 427, 429, 431, 433, 435, 437, 439, 441, 443, 445,
            447, 449, 451, 453, 455, 457, 459, 461, 463, 465, 467, 469, 471, 473, 475, 477,
            479, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495,
            496, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511
        ]
        res = coefs[v & 0x7f]
        return float(res if (v & 0x80) else -res)

    def load_frame(self, fifo, frame_idx):
        self.filters[0]['B'] = self._decode_gc(fifo[0])
        self.filters[0]['F'] = self._decode_gc(fifo[1])
        self.amp             = self._decode_ga(fifo[2])
        self.filters[1]['B'] = self._decode_gc(fifo[3])
        self.filters[1]['F'] = self._decode_gc(fifo[4])
        self.pitch           = fifo[5]
        self.filters[2]['B'] = self._decode_gc(fifo[6])
        self.filters[2]['F'] = self._decode_gc(fifo[7])
        self.repeat          = fifo[8] & 0x3f
        self.voiced          = bool(fifo[8] & 0x40)
        self.filters[3]['B'] = self._decode_gc(fifo[9])
        self.filters[3]['F'] = self._decode_gc(fifo[10])
        self.filters[4]['B'] = self._decode_gc(fifo[11])
        self.filters[4]['F'] = self._decode_gc(fifo[12])
        self.filters[5]['B'] = self._decode_gc(fifo[13])
        self.filters[5]['F'] = self._decode_gc(fifo[14])
        
        if not args.quiet:
            coeffs_str = "  ".join([f"{int(f['F']):4d} {int(f['B']):4d}" for f in self.filters])
            print(f"{frame_idx:4d}  {self.amp:4d}  "
                  f"{self.pitch:4d}  {self.voiced:>1}  {coeffs_str}", file=sys.stderr)

        self.pcount = 0
        self.rcount = 0
        for f in self.filters:
            f['z1'] = f['z2'] = 0.0 # Reset filter memory for new frame

    def generate_samples(self):
        samples = []
        pitch_val = max(1, self.pitch)
        repeat_count = max(1, self.repeat)   # MAME: if(rcount >= repeat), rcount starts at 0

        while self.rcount < repeat_count:
            if self.voiced:
                z0 = float(self.amp) if self.pcount == 0 else 0.0
            else:
                if self.rng & 1:
                    z0 = float(self.amp)
                    self.rng ^= 0x24000
                else:
                    z0 = float(-self.amp)
                self.rng >>= 1

            for f in self.filters:
                z0 += ((f['z1'] * f['F']) / 256.0) + ((f['z2'] * f['B']) / 512.0)
                f['z2'] = f['z1']
                f['z1'] = z0
                if z0 > 1e6:  z0 =  1e6
                if z0 < -1e6: z0 = -1e6

            samples.append(z0 * 8.0)

            self.pcount += 1
            if self.pcount >= pitch_val:
                self.pcount = 0
                self.rcount += 1

        return samples

def convert_bin_to_wav(input_file, output_file):
    synth = SP0250()
    all_samples = []
    frame_idx = 0

    if not args.quiet:
        header = (f"\n{'Fr':>4}  {'Amp':>4}  "
                  f"{'PitD':>4}  {'V':>1}  "
                  f"{'F1':>9}  {'F2':>9}  {'F3':>9}  {'F4':>9}  {'F5':>9}  {'F6':>9}")
        print(header, file=sys.stderr)
        print("-" * 85, file=sys.stderr)

    with open(input_file, 'rb') as f:
        while True:
            chunk = f.read(15)
            if not chunk or len(chunk) < 15:
                break
            synth.load_frame(list(chunk), frame_idx)
            all_samples.extend(synth.generate_samples())
            frame_idx += 1

    if not all_samples:
        print("Error: No samples were generated.")
        sys.exit(1)

    # float64 to handle values before final clipping
    audio_data = np.array(all_samples, dtype=np.float64)
    
    # Hard clip to the signed 16-bit range
    audio_data = np.clip(audio_data, -32768, 32767)
    
    with wave.open(output_file, 'wb') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(9286) # Based on 3.12MHz / 336 in MAME
        wav_file.writeframes(audio_data.astype(np.int16).tobytes())
    
    print(f"\nSaved to {output_file}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Decode raw SP0250 LPC binary frames to WAV file.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("input", help="Input binary file (.lpc / .bin)")
    parser.add_argument("output",  help="Output WAV (mono 16-bit PCM)")
    parser.add_argument('--quiet', action='store_true', help='disable diagnostic logging')
    args = parser.parse_args()
    convert_bin_to_wav(args.input, args.output)

#!/usr/bin/env python3

'''
Encodes a WAV file as raw SP0250 LPC frames for Sega arcade hardware.

Follows SP0250 Applications Manual for 15-byte stream framing and filter
coefficients. Inspired by Patrick Collins's TMS5100 LPC sound synthesizer.

Does not use librosa which has proven to be illfitted to sub 1kHz frequencies
in voice processing.

Note, Sega G80's 8035 factory image uses a decompression scheme that isn't
well known so this produces just a raw SP0250 LPC stream that and that stream
must be played through the 8035 using a simple streamer application.        
'''

import argparse
import sys
import numpy as np
import wave
import struct
from math import gcd
from scipy.signal import resample_poly

# ===============================
# Configuration
# ===============================

# SP0250 clocks everything at its own rate.  All LPC analysis happens at
# SP0250_RATE so that pole frequencies computed here map directly to the
# correct SP0250 filter frequencies at playback.
SP0250_RATE  = 10000          # encoder / analysis sample rate (Hz)
LPC_ORDER    = 12
NUM_SECTIONS = 6

PRE_EMPH = 0.97

# Analysis window size (samples at SP0250_RATE).  Used for both LPC and
# amplitude.  Fixed at 20 ms regardless of pitch — large enough for stable
# autocorrelation at the lowest expected F0 (~80 Hz → 2.5 periods in 20 ms).
LPC_WINDOW = 200

# Silence gate.
SILENCE_THRESHOLD = 0.015

# Pole radius limit per section (F1→F6, ascending frequency order).
POLE_RADIUS_BY_SECTION = [0.97, 0.97, 0.93, 0.88, 0.82, 0.75]

# Decoder clock rate (SP0250: 3.12 MHz / 336).  All pitch values stored in
# the LPC stream are in DECODER samples, not encoder samples.
DECODER_RATE = 9286

# Original Sega ROMs always use repeat=1.  Unvoiced and silent frames use
# a fixed pitch of 64 decoder samples (≈ 6.9 ms of noise/silence per frame).
REPEAT             = 1
UNVOICED_PITCH_DEC = 64

# Advance (encoder samples) for one unvoiced/silent frame:
#   64 decoder samples × (10000/9286) ≈ 69 encoder samples
UNVOICED_ADVANCE = round(UNVOICED_PITCH_DEC * SP0250_RATE / DECODER_RATE)  # 69

# SP0250 coefficient limits
B_MIN = -0.999
B_MAX =  0.0


# ===============================
# SP0250 ROM coefficient table
# ===============================

SP0250_ROM: np.ndarray = np.array([
    0.000000, 0.017578, 0.033203, 0.048828, 0.064453, 0.080078, 0.095703,
    0.111328, 0.126953, 0.142578, 0.158203, 0.173828, 0.189453, 0.205078,
    0.220703, 0.236328, 0.251953, 0.267578, 0.283203, 0.298828, 0.314453,
    0.330078, 0.345703, 0.361328, 0.376953, 0.392578, 0.408203, 0.423828,
    0.439453, 0.455078, 0.470703, 0.486328, 0.501953, 0.517578, 0.533203,
    0.548828, 0.564453, 0.580078, 0.587891, 0.595703, 0.603516, 0.611328,
    0.619141, 0.626953, 0.634766, 0.642578, 0.650391, 0.658203, 0.666016,
    0.673828, 0.681641, 0.689453, 0.697266, 0.705078, 0.712891, 0.720703,
    0.728516, 0.736328, 0.744141, 0.751953, 0.759766, 0.767578, 0.775391,
    0.783203, 0.791016, 0.798828, 0.806641, 0.814453, 0.822266, 0.830078,
    0.833984, 0.837891, 0.841797, 0.845703, 0.849609, 0.853516, 0.857422,
    0.861328, 0.865234, 0.869141, 0.873047, 0.876953, 0.880859, 0.884766,
    0.888672, 0.892578, 0.896484, 0.900391, 0.904297, 0.908203, 0.912109,
    0.916016, 0.919922, 0.923828, 0.927734, 0.931641, 0.935547, 0.939453,
    0.941406, 0.943359, 0.945313, 0.947266, 0.949219, 0.951172, 0.953125,
    0.955078, 0.957031, 0.958984, 0.960938, 0.962891, 0.964844, 0.966797,
    0.968750, 0.970703, 0.972656, 0.974609, 0.976563, 0.978516, 0.980469,
    0.982422, 0.984375, 0.986328, 0.988281, 0.990234, 0.992188, 0.994141,
    0.996094, 0.998047,
], dtype=np.float64)


def sp0250_quantize(value: float) -> int:
    """
    Encode a float coefficient to the SP0250 8-bit format:
        bit 7   : sign (1 = positive)
        bits 6:0: nearest-neighbour index into SP0250_ROM
    """
    sign      = 1 if value >= 0.0 else 0
    magnitude = np.clip(abs(value), 0.0, SP0250_ROM[-1])
    idx       = int(np.argmin(np.abs(SP0250_ROM - magnitude)))
    return (sign << 7) | idx


# ===============================
# WAV loader + resampler
# ===============================

def load_wav(filename: str):
    """
    Load a WAV file and resample to SP0250_RATE (10 kHz).
    Returns (samples_float32, SP0250_RATE).
    """
    with wave.open(filename, 'rb') as wf:
        src_rate = wf.getframerate()
        n        = wf.getnframes()
        data     = wf.readframes(n)
        samples  = struct.unpack('<' + 'h' * n, data)
        samples  = np.array(samples, dtype=np.float32) / 32768.0

    if src_rate != SP0250_RATE:
        g   = gcd(SP0250_RATE, src_rate)
        up  = SP0250_RATE // g
        dn  = src_rate    // g
        print(f"  Resampling {src_rate} Hz → {SP0250_RATE} Hz "
              f"(up={up}, down={dn})", file=sys.stderr)
        samples = resample_poly(samples, up, dn).astype(np.float32)

    return samples, SP0250_RATE


# ===============================
# LPC via Levinson-Durbin
# ===============================

def lpc_levinson(x: np.ndarray, order: int) -> np.ndarray:
    """
    Levinson-Durbin LPC.  Guarantees all poles inside unit circle.
    Returns polynomial [1, a1, ..., a_order].
    """
    x = np.asarray(x, dtype=np.float64)
    N = len(x)
    r    = np.array([np.dot(x[:N-k], x[k:]) for k in range(order + 1)])
    a    = np.zeros(order + 1)
    a[0] = 1.0
    e    = r[0]
    if e <= 0:
        return a
    for i in range(1, order + 1):
        acc = sum(a[j] * r[i - j] for j in range(1, i))
        k   = -(r[i] + acc) / e
        a_new = a.copy()
        for j in range(1, i):
            a_new[j] = a[j] + k * a[i - j]
        a_new[i] = k
        a = a_new
        e *= (1.0 - k * k)
        if e <= 1e-12:
            break
    return a


# ===============================
# Root extraction
# ===============================

def find_complex_pairs(roots) -> list:
    """
    Return the upper-half-plane representative of each stable complex
    conjugate pair (imag > 0, |r| < 1.0).
    Real roots are discarded: they produce B_t > 0 which violates SP0250.
    """
    upper = [r for r in roots if np.imag(r) > 1e-6 and np.abs(r) < 1.0]
    return [r for r in upper
            if any(np.allclose(np.conj(r), s, atol=1e-5)
                   for s in roots if np.abs(s) < 1.0)]


# ===============================
# Pole → (F_t, B_t)
# ===============================

def pole_to_FB(pole) -> tuple:
    """z = r·e^{jθ}  →  F_t = r·cosθ,  B_t = −r²"""
    r     = np.abs(pole)
    theta = np.angle(pole)
    return float(r * np.cos(theta)), float(-r * r)


# ===============================
# Enforce datasheet constraints
# ===============================

def enforce_constraints(F_t: float, B_t: float) -> tuple:
    """SP0250 datasheet:  −1 < B_t ≤ 0,  |F_t| ≤ √(−B_t)"""
    B_t   = float(np.clip(B_t, B_MIN, B_MAX))
    limit = np.sqrt(max(0.0, -B_t))
    F_t   = float(np.clip(F_t, -limit, limit))
    return F_t, B_t


# ===============================
# Amplitude encoding
# ===============================

def encode_amplitude(frame: np.ndarray) -> int:
    """
    SP0250 amplitude byte:  bits 7:5 = EXP,  bits 4:0 = MANT
        actual_amplitude = MANT << EXP    (max = 31 << 7 = 3968)
    """
    rms    = float(np.sqrt(np.mean(frame ** 2)))
    linear = int(rms * 32767)
    linear = max(0, min(linear, 3968))
    if linear == 0:
        return 0
    exp = 0
    while exp < 7 and (linear >> exp) > 31:
        exp += 1
    return ((exp & 0x07) << 5) | (min(linear >> exp, 31) & 0x1F)


# ===============================
# Pitch estimation
# ===============================

def estimate_pitch(frame_ext: np.ndarray, sample_rate: int) -> tuple:
    """
    Estimate pitch and voiced/unvoiced from a wide analysis window.

    Returns (f0_hz, is_voiced).
    f0_hz = 0 if unvoiced.

    Algorithm: Normalized Square Difference Function (NSDF), the core of
    the McLeod pitch method.  This is a time-domain autocorrelation variant
    that is immune to the frequency-resolution and harmonic-aliasing problems
    that cause HPS to produce octave errors on sustained tones.

    NSDF(τ) = 2 · r(τ) / (r(0) + r(τ,τ))

    where r(τ) is the unnormalized autocorrelation at lag τ and r(τ,τ) is
    the sum of squares of the shifted segment.  NSDF peaks at 1.0 for a
    perfectly periodic signal and ranges in [-1, 1].

    Voiced/Unvoiced decision: the peak NSDF value must exceed a threshold
    (typically 0.3–0.5).  Periodic voiced frames produce high peaks; noise
    and silence produce low, noisy NSDF.  ZCR is not used because it is
    unreliable on synthesized or re-encoded SP0250 audio.
    """
    x   = np.asarray(frame_ext, dtype=np.float64)
    N   = len(x)
    ste = float(np.mean(x ** 2))
    if ste < 1e-6:
        return 0, False

    # Lag search range: f_lo=60 Hz → max_lag, f_hi=500 Hz → min_lag
    min_lag = max(1,  int(sample_rate / 500))   # 20 @ 10 kHz
    max_lag = min(N - 1, int(sample_rate / 60)) # 166 @ 10 kHz

    # NSDF via FFT autocorrelation (O(N log N))
    # Zero-pad to avoid circular wrap-around
    fft_len  = 1 << (2 * N - 1).bit_length()    # next power of two ≥ 2N-1
    X        = np.fft.rfft(x, n=fft_len)
    acf_full = np.fft.irfft(X * np.conj(X))[:N] # r(τ) for τ=0..N-1

    # Cumulative sum-of-squares for normalisation denominator
    # running_ss[τ] = sum_{n=τ}^{N-1} x[n]^2  (the "right window" energy)
    x2        = x ** 2
    right_ss  = np.cumsum(x2[::-1])[::-1]       # right_ss[τ] = Σ x[n]² for n≥τ
    left_ss   = np.empty(N)
    left_ss[0] = 0.0
    # left_ss[τ] = Σ x[n]² for n=0..τ-1
    cs        = np.cumsum(x2)
    left_ss[1:] = cs[:-1]
    # denominator: energy of frame + energy of lagged frame
    denom     = right_ss + (right_ss[0] - left_ss)  # r(0,0) + r(τ,τ)
    denom     = np.where(denom < 1e-12, 1e-12, denom)

    nsdf = 2.0 * acf_full / denom  # NSDF in [-1, 1]

    # Find the best peak in the lag search range.
    # McLeod: take the highest peak above a threshold AFTER the first
    # zero-crossing (i.e., after the parabolic "trough" near lag 0).
    VOICED_THRESH = 0.45   # minimum NSDF peak to be called voiced

    nsdf_range = nsdf[min_lag:max_lag + 1]
    if nsdf_range.max() < VOICED_THRESH:
        return 0, False

    # Find the first zero-crossing from positive to negative, then look
    # for peaks after it so we skip the central lobe.
    signs        = np.sign(nsdf_range)
    zero_cross   = np.where((signs[:-1] > 0) & (signs[1:] <= 0))[0]
    search_start = int(zero_cross[0]) + 1 if len(zero_cross) else 0

    sub = nsdf_range[search_start:]
    if len(sub) == 0 or sub.max() < VOICED_THRESH:
        return 0, False

    best_lag = min_lag + search_start + int(np.argmax(sub))
    f0_hz    = float(sample_rate) / best_lag

    # ── Sub-octave preference ──────────────────────────────────────────────
    # If the NSDF also has a strong peak at 2× the detected lag, the shorter
    # lag is likely the 2nd harmonic, not the fundamental.  Prefer the longer
    # period (lower F0) whenever the doubled lag is still in range and its
    # NSDF value clears the voiced threshold.
    SUBOCTAVE_THRESH = 0.40
    for _ in range(2):          # up to 2 doublings (×4 in period)
        double_lag = best_lag * 2
        if double_lag > max_lag:
            break
        if nsdf[double_lag] >= SUBOCTAVE_THRESH:
            best_lag = double_lag
            f0_hz    = float(sample_rate) / best_lag
        else:
            break

    return f0_hz, True


# ===============================
# LPC analysis → SP0250 sections
# ===============================

def analyze_lpc(window: np.ndarray, pitch_enc: int) -> tuple:
    """
    Run LPC on a pre-emphasized, pitch-whitened, Hamming-windowed signal.
    Returns (sections, freqs) where sections is a list of (B_byte, F_byte)
    and freqs is a list of Hz values for diagnostic display.
    """
    # Pre-emphasis
    frame_pre = np.append(window[0], window[1:] - PRE_EMPH * window[:-1])

    # Pitch pre-whitening (voiced frames only)
    LTP_ALPHA = 0.7
    if pitch_enc > 0 and pitch_enc < len(frame_pre):
        whitened = frame_pre.copy()
        whitened[pitch_enc:] -= LTP_ALPHA * frame_pre[:-pitch_enc]
        frame_pre = whitened

    frame_win = frame_pre * np.hamming(len(frame_pre))

    lpc   = lpc_levinson(frame_win, LPC_ORDER)
    roots = np.roots(lpc)
    poles = find_complex_pairs(roots)
    poles.sort(key=lambda p: np.angle(p))

    sections = []
    freqs    = []
    for section_idx, pole in enumerate(poles[:NUM_SECTIONS]):
        r_max = POLE_RADIUS_BY_SECTION[section_idx]
        r     = min(float(np.abs(pole)), r_max)
        pole  = r * np.exp(1j * float(np.angle(pole)))

        F_t, B_t = pole_to_FB(pole)
        F_t, B_t = enforce_constraints(F_t, B_t)
        sections.append((sp0250_quantize(B_t), sp0250_quantize(F_t)))

        denom = np.sqrt(max(1e-9, -B_t))
        freqs.append(int(SP0250_RATE * np.arccos(
            np.clip(F_t / denom, -1.0, 1.0)) / (2 * np.pi)))

    while len(sections) < NUM_SECTIONS:
        sections.append((sp0250_quantize(0.0), sp0250_quantize(0.0)))
        freqs.append(0)

    return sections, freqs


# ===============================
# Frame builders
# ===============================

def _build_frame(sections, amp_byte: int, pitch_dec: int,
                 repeat: int, voiced: bool) -> bytes:
    rv = (0 << 7) | (int(voiced) << 6) | (repeat & 0x3F)
    return bytes([
        sections[0][0], sections[0][1], amp_byte,
        sections[1][0], sections[1][1], pitch_dec & 0xFF,
        sections[2][0], sections[2][1], rv,
        sections[3][0], sections[3][1],
        sections[4][0], sections[4][1],
        sections[5][0], sections[5][1],
    ])

# Null sections for silent/unvoiced frames (all coefficients zero)
_ZERO_SECTIONS = [(sp0250_quantize(0.0), sp0250_quantize(0.0))] * NUM_SECTIONS

def make_sentinel_frame() -> bytes:
    """Start-of-utterance: pitch=255, voiced=True, amp=0."""
    return _build_frame(_ZERO_SECTIONS, 0x00, 255, REPEAT, True)

def make_silent_frame() -> bytes:
    """
    Silent frame matching original Sega convention:
    voiced=True, pitch=64 (decoder), amp=0.
    The voiced flag with zero amplitude produces clean silence while
    keeping the decoder's filter state stable for smooth transitions.
    """
    return _build_frame(_ZERO_SECTIONS, 0x00, UNVOICED_PITCH_DEC, REPEAT, True)

def make_unvoiced_frame(sections, amp_byte: int) -> bytes:
    """Unvoiced: pitch=64, repeat=1, voiced=False."""
    return _build_frame(sections, amp_byte, UNVOICED_PITCH_DEC, REPEAT, False)

def make_voiced_frame(sections, amp_byte: int, pitch_dec: int) -> bytes:
    """Voiced: actual pitch period (decoder samples), repeat=1, voiced=True."""
    return _build_frame(sections, amp_byte, pitch_dec, REPEAT, True)


# ===============================
# Full file encode
# ===============================

def encode_wav_to_sp0250(wavfile: str, outfile: str) -> None:
    """
    Encode a WAV file to a raw SP0250 LPC bitstream.

    Frame emission matches original Sega ROM convention:
      • repeat = 1 always
      • Voiced:   one frame per pitch period (pitch-synchronous)
                  pitch stored in decoder samples (9286 Hz)
      • Unvoiced: pitch=64, voiced=False, advance 69 encoder samples
      • Silent:   pitch=64, voiced=True, amp=0, advance 69 encoder samples
      • Header:   one sentinel frame (pitch=255, voiced=True, amp=0)
      • Footer:   one all-zero null frame (end-of-utterance)
    """
    samples, sr = load_wav(wavfile)

    # Pitch window: 3× LPC_WINDOW centered on current position.
    PITCH_HALF = LPC_WINDOW         # half-width of extended window (one side)

    # Pad signal so windows never go out of bounds at edges.
    pad     = LPC_WINDOW
    padded  = np.concatenate([np.zeros(pad, np.float32),
                               samples,
                               np.zeros(pad, np.float32)])

    print(f"Input:  {wavfile}  ({sr} Hz, {len(samples)} samples, "
          f"{len(samples)/sr*1000:.0f} ms)", file=sys.stderr)

    header = (f"\n{'Fr':>4}  {'RMS':>7}  {'T':>1}  {'Amp':>4}  "
              f"{'PitD':>4}  {'V':>1}  "
              f"{'F1':>5}  {'F2':>5}  {'F3':>5}  {'F4':>5}  {'F5':>5}  {'F6':>5}")
    print(header, file=sys.stderr)
    print("-" * 75, file=sys.stderr)

    output_frames = []
    frame_idx     = 0

    # Start-of-utterance sentinel
    output_frames.append(make_sentinel_frame())
    print(f"{'  S':>4}  {'---':>7}  {'*':>1}  {'  0':>4}  {'255':>4}  "
          f"{'1':>1}  (sentinel)", file=sys.stderr)

    pos = 0   # current position in encoder (10 kHz) samples

    while pos < len(samples):
        # ── Analysis window (LPC + amplitude) ──────────────────────────────
        # Fixed LPC_WINDOW samples, clipped at file boundaries.
        win_start = pos
        win_end   = min(pos + LPC_WINDOW, len(samples))
        window    = samples[win_start:win_end]
        if len(window) < LPC_WINDOW:
            window = np.pad(window, (0, LPC_WINDOW - len(window)))

        rms = float(np.sqrt(np.mean(window ** 2)))

        # ── Pitch/V·U (wider window) ────────────────────────────────────────
        p_start  = max(0, pos - PITCH_HALF) + pad
        p_end    = min(pos + PITCH_HALF + LPC_WINDOW, len(samples)) + pad
        win_ext  = padded[p_start:p_end]
        f0_hz, voiced = estimate_pitch(win_ext, sr)

        # ── Emit frame ──────────────────────────────────────────────────────
        if rms < SILENCE_THRESHOLD:
            fb = make_silent_frame()
            advance  = UNVOICED_ADVANCE
            diag_t   = 'S'
            diag_pit = UNVOICED_PITCH_DEC
            diag_v   = 1
            diag_amp = 0
            diag_f   = []

        elif not voiced:
            sections, freqs = analyze_lpc(window, 0)
            amp_byte        = encode_amplitude(window)
            fb              = make_unvoiced_frame(sections, amp_byte)
            advance         = UNVOICED_ADVANCE
            diag_t          = 'U'
            diag_pit        = UNVOICED_PITCH_DEC
            diag_v          = 0
            diag_amp        = (amp_byte & 0x1F) << ((amp_byte >> 5) & 7)
            diag_f          = freqs

        else:
            # Convert F0 to pitch period in encoder samples and decoder samples
            pitch_enc = max(1, int(round(sr           / f0_hz)))
            pitch_dec = max(1, min(255, int(round(DECODER_RATE / f0_hz))))

            sections, freqs = analyze_lpc(window, pitch_enc)
            amp_byte        = encode_amplitude(window)
            fb              = make_voiced_frame(sections, amp_byte, pitch_dec)
            advance         = pitch_enc
            diag_t          = 'V'
            diag_pit        = pitch_dec
            diag_v          = 1
            diag_amp        = (amp_byte & 0x1F) << ((amp_byte >> 5) & 7)
            diag_f          = freqs

        assert len(fb) == 15
        output_frames.append(fb)

        fstr = '  '.join(f'{hz:5d}' for hz in diag_f) if diag_f else ''
        print(f"{frame_idx:4d}  {rms:7.4f}  {diag_t:>1}  {diag_amp:4d}  "
              f"{diag_pit:4d}  {diag_v}  {fstr}", file=sys.stderr)

        frame_idx += 1
        pos       += advance

    # End-of-utterance null frame
    output_frames.append(b'\x00' * 15)

    with open(outfile, 'wb') as f:
        for fb in output_frames:
            f.write(fb)

    n_speech = len(output_frames) - 2  # exclude sentinel and null
    total    = len(output_frames) * 15
    print(f"\nOutput: {outfile}  "
          f"({n_speech} speech frames + sentinel + null = {total} bytes)",
          file=sys.stderr)

    # Hex dump
    print(f"\n{'Fr':>4}  B1 F1 Am B2 F2 Pt B3 F3 RV B4 F4 B5 F5 B6 F6",
          file=sys.stderr)
    print("-" * 52, file=sys.stderr)
    with open(outfile, 'rb') as f:
        for ix in range(len(output_frames)):
            chunk = f.read(15)
            if len(chunk) < 15:
                break
            print(f"{ix:4d}  " + ' '.join(f'{b:02x}' for b in chunk),
                  file=sys.stderr)


# ===============================
# Entry point
# ===============================

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Encode WAV to raw SP0250 LPC binary frames.\n"
                    "Input is resampled to 10 kHz (SP0250 native rate).\n"
                    "All diagnostic output goes to stderr.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("input",  help="Input WAV (any rate, mono 16-bit PCM)")
    parser.add_argument("output", help="Output binary file (.lpc / .bin)")
    args = parser.parse_args()
    encode_wav_to_sp0250(args.input, args.output)

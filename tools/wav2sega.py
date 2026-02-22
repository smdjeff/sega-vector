#!/usr/bin/env python3

'''
Encodes a WAV file as raw SP0250 LPC frames for Sega arcade hardware.

Follows SP0250 Applications Manual for 15-byte stream framing and filter
coefficients. Inspired by Patrick Collins's TMS5100 LPC sound synthesizer.

Uses librosa, which is a much more powerful music and audio analysis library
but because it's broad spectrum, it has more trouble finding voice pitch 
than vintage algorithms.

Timing note: Generally produces SP0250 streams that are have as long as vintage
algorithms. This is because repeat is usually 2 instead of 1. That's because 
SP0250 playback duration per frame isn't a fixed 20ms but pitch_period * repeat samples

Voiced to unvoiced transition notes:
  V->U: The SP0250 filter has state. If we switch excitation (pulse->noise) while
        the filter is energised, the discontinuity produces a click. We zero the
        amplitude of the last voiced frame before every V->U boundary.
  U->V: The reference encoder always inserts a voiced, amp=0, pitch=8 "drain"
        frame after every unvoiced region. This lets the filter state decay before
        real voiced synthesis starts, avoiding a thump on re-entry.
  Unvoiced pitch: Always fixed to 64. SP0250 clocks its noise source from the
        pitch register even in unvoiced mode — a small value makes the noise
        periodic enough to sound like a tone.

TODO: 
    Currently Sega G80's 8035 decompression scheme is unknown so
    input file must be decompressed SP0250 LPC from MAME dump.        
'''

import numpy as np
import librosa
import argparse
from scipy import signal

SP0250_COEFS = [
      0,   9,  17,  25,  33,  41,  49,  57,  65,  73,  81,  89,  97, 105, 113, 121,
    129, 137, 145, 153, 161, 169, 177, 185, 193, 201, 209, 217, 225, 233, 241, 249,
    257, 265, 273, 281, 289, 297, 301, 305, 309, 313, 317, 321, 325, 329, 333, 337,
    341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381, 385, 389, 393, 397, 401,
    405, 409, 413, 417, 421, 425, 427, 429, 431, 433, 435, 437, 439, 441, 443, 445,
    447, 449, 451, 453, 455, 457, 459, 461, 463, 467, 469, 471, 473, 475, 477,
    479, 481, 482, 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495,
    496, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511
]
_COEF_ARR = np.array(SP0250_COEFS, dtype=np.float32)

UNVOICED_PITCH = 64   # noise clock rate for unvoiced frames
DRAIN_PITCH    = 8    # U->V drain frame pitch (amp=0, lets filter decay)


def encode_gc(val):
    sign_bit = 0x80 if val >= 0 else 0x00
    idx = int((np.abs(_COEF_ARR - abs(val) * 512.0)).argmin())
    return sign_bit | (idx & 0x7F)


def encode_amplitude(target_gain):
    target_gain = np.clip(target_gain, 0, 3968)
    for e in range(7, -1, -1):
        m = int(target_gain / (1 << e))
        if m <= 31:
            return (e << 5) | (m & 0x1F)
    return 0


def apply_bandpass(y, sr, low, high, order=8):
    nyq = 0.5 * sr
    b, a = signal.butter(order, [low / nyq, high / nyq], btype='band')
    return signal.filtfilt(b, a, y)


def yin_pitch(frame, sr, fmin=60.0, fmax=400.0, threshold=0.15):
    """
    YIN pitch estimator (de Cheveigne & Kawahara, 2002).

    Uses the Cumulative Mean Normalized Difference function (CMNDF) whose
    first minimum below threshold is the fundamental — harmonics only appear
    at later, larger minima so they are naturally avoided.

    Analysis window is 2*hi samples (where hi = sr/fmin) to ensure every lag
    has a full non-overlapping comparison window.

    Returns (pitch_samples, voiced_strength) where voiced_strength = 1 - CMNDF_min.
    """
    lo = max(2, int(sr / fmax))
    hi = min(len(frame) // 2, int(sr / fmin))
    if lo >= hi:
        return None, 0.0

    W = 2 * hi
    analysis = frame[:W] if len(frame) >= W else np.pad(frame, (0, W - len(frame)))

    fft_size = 1
    while fft_size < 2 * W:
        fft_size <<= 1
    F   = np.fft.rfft(analysis, n=fft_size)
    acf = np.fft.irfft(F * np.conj(F))[:W]

    diff = np.zeros(hi + 1)
    for tau in range(1, hi + 1):
        diff[tau] = 2.0 * acf[0] - 2.0 * acf[tau]

    cmndf = np.ones(hi + 1)
    running_sum = 0.0
    for tau in range(1, hi + 1):
        running_sum += diff[tau]
        cmndf[tau] = diff[tau] * tau / running_sum if running_sum > 0 else 1.0

    tau_star = None
    for tau in range(lo, hi):
        if cmndf[tau] < threshold:
            while tau + 1 <= hi and cmndf[tau + 1] < cmndf[tau]:
                tau += 1
            tau_star = tau
            break
    if tau_star is None:
        tau_star = lo + int(np.argmin(cmndf[lo:hi + 1]))

    if 0 < tau_star < hi:
        y0, y1, y2 = cmndf[tau_star - 1], cmndf[tau_star], cmndf[tau_star + 1]
        denom = 2.0 * (2.0 * y1 - y0 - y2)
        if denom > 1e-8:
            tau_star = tau_star + (y0 - y2) / denom

    voiced_strength = max(0.0, 1.0 - float(cmndf[int(round(tau_star))]))
    return float(tau_star), voiced_strength


def smooth_voiced(voiced_raw, min_run=2):
    """
    Remove voiced/unvoiced runs shorter than min_run by merging into the
    preceding run. Single forward pass — cannot hang.
    """
    voiced = list(voiced_raw)
    n = len(voiced)
    i = 0
    while i < n:
        j = i
        while j < n and voiced[j] == voiced[i]:
            j += 1
        if (j - i) < min_run and i > 0:
            for k in range(i, j):
                voiced[k] = voiced[i - 1]
        i = j
    return voiced


def apply_transition_bridging(frames):
    """
    Post-processing pass to clean up V<->U excitation switches.

    V->U: Zero the amplitude of the last voiced frame before each transition.
          The SP0250 filter has state — switching excitation while the filter
          is energised causes a click. Draining first prevents this.

    U->V: Set the first voiced frame after each unvoiced region to amp=0,
          pitch=DRAIN_PITCH. This matches the reference encoder's pattern and
          lets the filter state decay fully before real voiced synthesis starts.
    """
    n = len(frames)
    for i in range(1, n):
        prev_voiced = bool(frames[i-1][8] & 0x40)
        this_voiced = bool(frames[i][8]   & 0x40)

        if prev_voiced and not this_voiced:
            # V->U: silence the last voiced frame
            frames[i-1][2] = 0   # amp = 0

        if not prev_voiced and this_voiced:
            # U->V: turn this frame into a silent drain frame
            frames[i][2] = 0           # amp = 0
            frames[i][5] = DRAIN_PITCH # pitch = 8
            # repeat stays as-is (short, since pitch is small)
            repeat = max(1, min(63, frames[i][8] & 0x3F))
            frames[i][8] = 0x40 | repeat   # voiced, same repeat

    return frames


def wav_to_sp0250(input_wav, output_bin, pre_emph, low, high, gate, order, v_bias, z_thresh):
    sr_target = 9286
    hop_length   = int(sr_target * 20 / 1000)  # ~185 samples = 20ms
    frame_length = hop_length * 2               # ~370 samples = 40ms, 50% overlap

    y, sr = librosa.load(input_wav, sr=sr_target, mono=True)
    y = apply_bandpass(y, sr, low=low, high=high)

    y_raw = y.copy()   # pre-emphasis suppresses the fundamental; keep raw for pitch

    if pre_emph > 0:
        y_emph = librosa.effects.preemphasis(y, coef=pre_emph)
    else:
        y_emph = y
    y_emph = librosa.util.normalize(y_emph) * 0.95

    f0, voiced_flag, voiced_probs = librosa.pyin(
        y_raw,
        fmin=60, fmax=400, sr=sr,
        hop_length=hop_length,
        frame_length=frame_length,
        fill_na=0.0,
    )

    frames_emph = librosa.util.frame(y_emph, frame_length=frame_length, hop_length=hop_length)
    frames_raw  = librosa.util.frame(y_raw,  frame_length=frame_length, hop_length=hop_length)
    n_frames    = min(frames_emph.shape[1], frames_raw.shape[1], len(f0))

    # ── Pass 1: voiced decision + pitch ──────────────────────────────────────
    raw_voiced  = []
    pitch_track = []
    last_pitch  = 100.0

    for i in range(n_frames):
        frame_emph = frames_emph[:, i]
        frame_raw  = frames_raw[:, i]
        rms_val    = float(np.sqrt(np.mean(frame_emph ** 2)))
        zcr        = float(np.mean(librosa.feature.zero_crossing_rate(y=frame_emph)))

        if rms_val < gate:
            raw_voiced.append(False)   # silence is always unvoiced
            pitch_track.append(np.nan)
            continue

        pyin_v    = bool(voiced_flag[i]) if i < len(voiced_flag) else False
        pyin_prob = (
            float(voiced_probs[i])
            if (voiced_probs is not None and i < len(voiced_probs)
                and not np.isnan(voiced_probs[i]))
            else 0.0
        )

        yin_period, yin_strength = yin_pitch(frame_raw, sr, fmin=60.0, fmax=400.0)

        if zcr > z_thresh:
            raw_voiced.append(False)
            pitch_track.append(np.nan)
        else:
            if pyin_v and pyin_prob > 0.3:
                raw_voiced.append(True)
            elif yin_strength > (0.35 - v_bias):
                raw_voiced.append(True)
            else:
                raw_voiced.append(False)

            if f0[i] > 0:
                p = sr / f0[i]
                pitch_track.append(p)
                last_pitch = p
            else:
                # No YIN fallback — YIN frequently locks onto harmonics.
                # Leave as NaN for the interpolation pass below.
                pitch_track.append(np.nan)

    # Interpolate NaN pitch gaps
    pitch_arr  = np.array(pitch_track, dtype=float)
    nans_orig  = np.isnan(pitch_arr).copy()   # which frames had no pyin estimate
    nans       = nans_orig.copy()
    if nans.any() and (~nans).any():
        x = np.arange(len(pitch_arr))
        pitch_arr[nans] = np.interp(x[nans], x[~nans], pitch_arr[~nans])
    elif nans.all():
        pitch_arr[:] = last_pitch

    # np.interp clamps trailing NaN frames (after the last good pyin estimate)
    # to the last known value, producing an unnaturally flat pitch tail.
    # Replace those frames with a linear extrapolation of the recent trend.
    # Natural speech shows sentence-final declination: pitch falls (period rises)
    # at the end of declarative phrases, so we enforce at least a gentle slope.
    good_idxs = np.where(~nans_orig)[0]
    if len(good_idxs) > 1:
        tail_start = int(good_idxs[-1]) + 1
        if tail_start < len(pitch_arr) and nans_orig[tail_start:].any():
            lookback   = min(8, len(good_idxs))
            recent_i   = good_idxs[-lookback:].astype(float)
            recent_v   = pitch_arr[good_idxs[-lookback:]]
            slope      = float(np.polyfit(recent_i, recent_v, 1)[0])
            # Ensure at least +1 sample/frame (≈1 Hz fall near 95 Hz) so the
            # tail doesn't sound flat or rising relative to the rest of the phrase.
            slope      = max(slope, 1.0)
            for j in range(tail_start, len(pitch_arr)):
                if nans_orig[j]:
                    pitch_arr[j] = np.clip(pitch_arr[j - 1] + slope, 16, 255)

    smoothed_voiced = smooth_voiced(raw_voiced, min_run=2)

    # ── Pass 2: LPC + pack frames ─────────────────────────────────────────────
    encoded_frames = []
    voiced_count   = 0
    active_count   = 0

    for i in range(n_frames):
        frame_emph = frames_emph[:, i]
        rms_val    = float(np.sqrt(np.mean(frame_emph ** 2)))
        is_voiced  = smoothed_voiced[i]
        pitch_val  = float(pitch_arr[i])

        if rms_val < gate:
            amp_byte = 0
        else:
            active_count += 1
            amp_byte = encode_amplitude(rms_val * 8000)
            if is_voiced:
                voiced_count += 1

        try:
            a = librosa.lpc(
                frame_emph + 1e-5 * np.random.randn(len(frame_emph)),
                order=order
            )
        except Exception:
            a    = np.zeros(order + 1)
            a[0] = 1.0

        if is_voiced:
            roots = np.roots(a)
            roots[np.abs(roots) >= 1] *= 0.985

            fb_pairs   = []
            roots_list = list(roots)
            while len(roots_list) >= 2 and len(fb_pairs) < 6:
                r        = roots_list.pop(0)
                conj_idx = int(np.argmin([np.abs(rc - np.conj(r)) for rc in roots_list]))
                r_conj   = roots_list.pop(conj_idx)
                fb_pairs.append(((r + r_conj).real / 2.0, -(r * r_conj).real))
            while len(fb_pairs) < 6:
                fb_pairs.append((0.0, 0.0))
        else:
            # Unvoiced frames: flat coefficients keep the noise broadband.
            # Voiced-spectral LPC coefficients (B ≈ −1) passed to the LFSR
            # noise source produce a narrow-band screech ("scratch" artefact).
            fb_pairs = [(0.0, 0.0)] * 6

        if is_voiced:
            pitch_int = int(np.clip(round(pitch_val), 16, 255))
        else:
            pitch_int = UNVOICED_PITCH

        repeat = int(np.clip(round(hop_length / pitch_int), 1, 63))

        bin_frame      = bytearray(15)
        bin_frame[0]   = encode_gc(fb_pairs[0][1])
        bin_frame[1]   = encode_gc(fb_pairs[0][0])
        bin_frame[2]   = amp_byte
        bin_frame[3]   = encode_gc(fb_pairs[1][1])
        bin_frame[4]   = encode_gc(fb_pairs[1][0])
        bin_frame[5]   = pitch_int
        bin_frame[6]   = encode_gc(fb_pairs[2][1])
        bin_frame[7]   = encode_gc(fb_pairs[2][0])
        bin_frame[8]   = (0x40 if is_voiced else 0x00) | (repeat & 0x3F)
        bin_frame[9]   = encode_gc(fb_pairs[3][1])
        bin_frame[10]  = encode_gc(fb_pairs[3][0])
        bin_frame[11]  = encode_gc(fb_pairs[4][1])
        bin_frame[12]  = encode_gc(fb_pairs[4][0])
        bin_frame[13]  = encode_gc(fb_pairs[5][1])
        bin_frame[14]  = encode_gc(fb_pairs[5][0])
        encoded_frames.append(bin_frame)

    # ── Pass 3: transition bridging ───────────────────────────────────────────
    encoded_frames = apply_transition_bridging(encoded_frames)

    with open(output_bin, 'wb') as f:
        for fd in encoded_frames:
            f.write(fd)

    total_samples = sum(
        encoded_frames[i][5] * (encoded_frames[i][8] & 0x3F)
        for i in range(len(encoded_frames))
    )
    if active_count > 0:
        v_pct = (voiced_count / active_count) * 100
        print(f"Frames   : {len(encoded_frames)} total, {active_count} active")
        print(f"Voiced   : {voiced_count}/{active_count} = {v_pct:.1f}%")
        print(f"Estimated playback duration: {total_samples / sr_target:.3f}s")
    else:
        print("Warning: no active frames — check --gate value")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Encode WAV audio to raw SP0250 LPC binary frames"
    )
    parser.add_argument("input",  help="Input WAV file")
    parser.add_argument("output", help="Output binary file")
    parser.add_argument("--pre_emph", type=float, default=0.0,
        help="Pre-emphasis coefficient, applied to LPC only (default 0.0)")
    parser.add_argument("--low",  type=float, default=100.0,
        help="Bandpass low cutoff Hz (default 100)")
    parser.add_argument("--high", type=float, default=3200.0,
        help="Bandpass high cutoff Hz (default 3200)")
    parser.add_argument("--gate", type=float, default=0.03,
        help="RMS silence gate threshold (default 0.03)")
    parser.add_argument("--order", type=int, default=10,
        help="LPC order, SP0250 max is 12 (default 10)")
    parser.add_argument("--voice-bias", type=float, default=0.15,
        help="Adjusts YIN voiced_strength threshold (default 0.15). "
             "Positive = more voiced. Range roughly -0.3 to +0.3.")
    parser.add_argument("--z-thresh", type=float, default=0.25,
        help="ZCR threshold for sibilant/fricative protection (default 0.25)")
    args = parser.parse_args()

    wav_to_sp0250(
        args.input, args.output,
        args.pre_emph, args.low, args.high,
        args.gate, args.order,
        args.voice_bias, args.z_thresh
    )
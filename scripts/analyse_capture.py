"""Reproduce W05 measurements from saved sigrok samples and same-reset serial.

Uses only the Python standard library. D0 is SDA, D1 is SCL. This is an
offline check of the saved samples; it neither acquires USB data nor calibrates
the FX2 clock. PulseView cursor screenshots remain the instrument readouts.
"""

import argparse
import configparser
import hashlib
import json
import re
import statistics
import struct
from pathlib import Path
from zipfile import ZipFile


def read_capture(path):
    with ZipFile(path) as archive:
        metadata = configparser.ConfigParser()
        metadata.read_string(archive.read("metadata").decode("utf-8"))
        device = metadata["device 1"]
        if device.getint("unitsize") != 1:
            raise ValueError("Only one-byte digital samples are supported")
        rate = re.fullmatch(r"([\d.]+)\s*([kM]?)Hz", device["samplerate"])
        if not rate:
            raise ValueError("Unrecognized sample rate")
        hz = float(rate[1]) * {"": 1, "k": 1000, "M": 1000000}[rate[2]]
        prefix = device["capturefile"] + "-"
        chunks = [n for n in archive.namelist() if n.startswith(prefix)]
        chunks.sort(key=lambda n: int(n[len(prefix):]))
        samples = b"".join(archive.read(n) for n in chunks)
    if len(samples) < 2 or hz <= 0:
        raise ValueError("Capture contains no usable samples")
    return samples, hz


def decode_i2c(samples):
    segments = []
    current = None
    bits = []
    previous = samples[0] & 3

    def finish(index, condition):
        if current is not None:
            current.update(end_sample=index, end=condition, partial_bits=bits[:])
            segments.append(current)

    for index, sample in enumerate(samples[1:], 1):
        state = sample & 3
        if state == previous:
            continue
        sda, scl = state & 1, (state >> 1) & 1
        old_sda, old_scl = previous & 1, (previous >> 1) & 1
        if scl and old_scl and sda != old_sda:
            if not sda:
                finish(index, "repeated_start")
                current = dict(start_sample=index, bytes=[], acks=[], rising_samples=[])
                bits = []
            else:
                finish(index, "stop")
                current = None
                bits = []
        elif scl and not old_scl and current is not None:
            bits.append(sda)
            current["rising_samples"].append(index)
            if len(bits) == 9:
                value = 0
                for bit in bits[:8]:
                    value = (value << 1) | bit
                current["bytes"].append(value)
                current["acks"].append("NACK" if bits[8] else "ACK")
                bits = []
        previous = state
    return segments


def analyse(capture, serial):
    samples, hz = read_capture(capture)
    segments = decode_i2c(samples)
    identity = next(i for i, s in enumerate(segments)
                    if s["bytes"] == [0xD0, 0x75] and s["end"] == "repeated_start")
    request, reply = segments[identity:identity + 2]
    if (request["acks"] != ["ACK", "ACK"]
            or reply["bytes"] != [0xD1, 0x68]
            or reply["acks"] != ["ACK", "NACK"] or reply["end"] != "stop"):
        raise ValueError("WHO_AM_I sequence does not match the prediction")

    output_index = next(i for i, s in enumerate(segments)
                        if s["bytes"] == [0xD0, 0x3B] and s["end"] == "repeated_start")
    output, frame_reply = segments[output_index:output_index + 2]
    config = [s for s in segments[:output_index]
              if s["bytes"] == [0xD0, 0x38, 0x01] and s["end"] == "stop"][-1]
    if frame_reply["bytes"][0] != 0xD1 or len(frame_reply["bytes"]) != 7:
        raise ValueError("First acceleration frame is incomplete")
    if output["acks"] != ["ACK", "ACK"] or frame_reply["acks"] != ["ACK"] * 6 + ["NACK"]:
        raise ValueError("First acceleration frame has unexpected ACK/NACK bits")
    frame = list(struct.unpack(">hhh", bytes(frame_reply["bytes"][1:])))

    log = serial.read_text(encoding="utf-8-sig")
    board = re.search(r"gap_us=(\d+) settled=1", log)
    first = re.search(r"^0,(-?\d+),(-?\d+),(-?\d+),", log, re.MULTILINE)
    if (not board or not first or "halt=" in log
            or "who_am_i=0x68 expected=0x68" not in log):
        raise ValueError("Serial log lacks the successful Sketch 4 preconditions")
    board_us = int(board[1])
    serial_frame = [int(v) for v in first.groups()]
    if frame != serial_frame:
        raise ValueError("Captured first frame differs from the serial frame")
    gap_us = (output["start_sample"] - config["end_sample"]) * 1e6 / hz
    if board_us <= 0 or gap_us <= 0:
        raise ValueError("Measured intervals must be positive")
    rising = request["rising_samples"]
    period_samples = rising[1] - rising[0]
    # Ignore byte-boundary pauses when checking the clock; report them separately.
    within_byte = [rising[i + 1] - rising[i]
                   for i in range(8) if i + 1 < len(rising)]
    difference = gap_us - board_us
    tolerance = max(20.0, 0.02 * board_us)  # unchanged course crosscheck.py policy
    return dict(
        capture=str(capture), serial=str(serial), sample_rate_hz=hz,
        sample_count=len(samples), sample_resolution_us=1e6 / hz,
        capture_sha256=hashlib.sha256(capture.read_bytes()).hexdigest(),
        raw_sha256=hashlib.sha256(samples).hexdigest(),
        serial_sha256=hashlib.sha256(serial.read_bytes()).hexdigest(),
        whoami_wire_hex=["D0", "75", "D1", "68"],
        whoami_ack_sequence=request["acks"] + reply["acks"],
        whoami_duration_us=(reply["end_sample"] - request["start_sample"]) * 1e6 / hz,
        scl_cursor_samples=rising[:2], scl_period_us=period_samples * 1e6 / hz,
        scl_frequency_hz=hz / period_samples,
        first_byte_median_period_us=statistics.median(within_byte) * 1e6 / hz,
        gap_cursor_samples=[config["end_sample"], output["start_sample"]],
        board_gap_us=board_us, analyser_gap_us=gap_us,
        difference_us=difference, difference_percent=100 * difference / board_us,
        tolerance_us=tolerance,
        verdict="AGREE" if abs(difference) <= tolerance else "DISAGREE",
        analyser_first_frame=frame, serial_first_frame=serial_frame,
        first_frame_matches=True,
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("serial", type=Path)
    parser.add_argument("--json", type=Path, help="optional report file")
    args = parser.parse_args()
    try:
        result = analyse(args.capture, args.serial)
    except (ValueError, KeyError, IndexError, StopIteration, OSError) as exc:
        parser.exit(2, f"Cannot validate capture: {exc}\n")
    report = json.dumps(result, indent=2) + "\n"
    if args.json:
        args.json.write_text(report, encoding="utf-8")
    print(report, end="")


if __name__ == "__main__":
    main()

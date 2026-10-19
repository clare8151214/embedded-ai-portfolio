"""Capture an already-loaded sigrok FX2 (8ch), without changing its driver.

USB protocol: sigrokproject/libsigrok src/hardware/fx2lafw/protocol.{c,h}.
Session format: https://sigrok.org/wiki/File_format:Sigrok/v2
The bytes saved here are instrument samples, never generated waveforms.
"""
import ctypes
import contextlib
import datetime
import hashlib
import json
import pathlib
import re
import sys
import threading
import time
import zipfile

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE / 'python-libs'))
import usb1
import serial

DLL = HERE / 'VS2022/MS64/dll/libusb-1.0.dll'
usb1.loadLibrary(ctypes.WinDLL(str(DLL)))
RATE = 2_000_000
LIMIT = RATE * 5
STAMP = '20261019-' + datetime.datetime.now().strftime('%H%M%S')
BASE = ROOT / 'w05' / ('whoami-' + STAMP)
serial_data = bytearray()
serial_errors = []


def reset_and_record(port):
    try:
        time.sleep(0.10)
        port.setDTR(False)
        port.setRTS(True)
        time.sleep(0.15)
        port.setRTS(False)
        port.setDTR(False)
        end = time.monotonic() + 15
        while time.monotonic() < end:
            chunk = port.read(max(1, port.in_waiting))
            serial_data.extend(chunk)
            if re.search(rb'(?m)^9,-?\d+,-?\d+,-?\d+,[^\r\n]+\r?\n', serial_data):
                break
            if re.search(rb'(?m)^halt=[^\r\n]+\r', serial_data):
                break
    except Exception as exc:
        serial_errors.append(repr(exc))


def main():
    chunks = []
    errors = []
    total = 0
    stopping = False
    transfers = []
    port = serial.Serial(port=None, baudrate=115200, timeout=0.1)
    port.port = 'COM6'
    port.dtr = False
    port.rts = False
    port.open()
    port.reset_input_buffer()
    thread = None
    try:
        with usb1.USBContext() as context:
            handle = context.openByVendorIDAndProductID(0x1d50, 0x608c, skip_on_error=False)
            if handle is None:
                raise RuntimeError('The sigrok FX2 VID:PID 1d50:608c was not found')
            with contextlib.closing(handle):
                handle.claimInterface(0)
                firmware = list(handle.controlRead(0xc0, 0xb0, 0, 0, 2, timeout=1000))
                if len(firmware) != 2 or firmware[0] != 1:
                    raise RuntimeError('Unsupported FX2 firmware: ' + repr(firmware))
                handle.setInterfaceAltSetting(0, 0)

                def receive(transfer):
                    nonlocal total, stopping
                    status = transfer.getStatus()
                    if status == usb1.TRANSFER_COMPLETED:
                        n = transfer.getActualLength()
                        if not stopping and n:
                            take = min(n, LIMIT - total)
                            chunks.append(bytes(transfer.getBuffer()[:take]))
                            total += take
                            if total >= LIMIT:
                                stopping = True
                        if not stopping:
                            transfer.submit()
                    elif status != usb1.TRANSFER_CANCELLED:
                        errors.append({'usb_transfer_status': status, 'actual_length': transfer.getActualLength()})
                        stopping = True

                for _ in range(32):
                    transfer = handle.getTransfer()
                    transfer.setBulk(0x82, 32768, callback=receive, timeout=8000)
                    transfer.submit()
                    transfers.append(transfer)
                # 48 MHz clock, 8-bit samples, divider = 23 + 1 -> 2 MHz.
                command = bytes([0x40, 0, 23])
                written = handle.controlWrite(0x40, 0xb1, 0, 0, command, timeout=1000)
                if written != 3:
                    raise RuntimeError('Incomplete FX2 start command')
                started = time.monotonic()
                print('CAPTURING 2 MHz, 10M samples; resetting COM6', flush=True)
                thread = threading.Thread(target=reset_and_record, args=(port,))
                thread.start()
                try:
                    while not stopping and time.monotonic() - started < 12:
                        context.handleEventsTimeout(0.1)
                finally:
                    stopping = True
                    for transfer in transfers:
                        if transfer.isSubmitted():
                            transfer.cancel()
                    deadline = time.monotonic() + 3
                    while any(t.isSubmitted() for t in transfers) and time.monotonic() < deadline:
                        context.handleEventsTimeout(0.05)
                    if any(t.isSubmitted() for t in transfers):
                        errors.append({'cleanup': 'pending transfers after cancellation timeout'})
                    for transfer in transfers:
                        transfer.close()
                elapsed = time.monotonic() - started
                handle.releaseInterface(0)
    finally:
        if thread is not None:
            thread.join(timeout=17)
        port.close()

    data = b''.join(chunks)
    BASE.with_suffix('.bin').write_bytes(data)
    BASE.with_name(BASE.name + '-serial.txt').write_bytes(serial_data)
    metadata = '[global]\nsigrok version=0.5.2\n\n[device 1]\ncapturefile=logic-1\nunitsize=1\ntotal probes=8\nsamplerate=2 MHz\n'
    metadata += ''.join('probe%d=D%d\n' % (i + 1, i) for i in range(8))
    with zipfile.ZipFile(BASE.with_suffix('.sr'), 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr('version', '2', compress_type=zipfile.ZIP_STORED)
        archive.writestr('metadata', metadata)
        archive.writestr('logic-1-1', data)
    report = {
        'client_date': '2026-10-19', 'capture_file': str(BASE.with_suffix('.sr')),
        'method': 'Physical FX2 asynchronous USB capture via libusb 1.0.30; PulseView native open failed',
        'sample_rate_hz': RATE, 'sample_count': len(data), 'duration_s': len(data) / RATE,
        'channels': {str(i): 'FX2 D%d' % i for i in range(8)},
        'expected_wiring_not_assumed_verified': {'D0': 'GPIO8/SDA', 'D1': 'GPIO9/SCL'},
        'firmware_version': firmware, 'start_command_hex': command.hex(),
        'libusb_version': str(usb1.getVersion()), 'dll_sha256': hashlib.sha256(DLL.read_bytes()).hexdigest(),
        'raw_sha256': hashlib.sha256(data).hexdigest(),
        'serial_sha256': hashlib.sha256(serial_data).hexdigest(),
        'host_capture_elapsed_s': elapsed, 'usb_errors': errors, 'serial_errors': serial_errors,
        'complete': len(data) == LIMIT and not errors,
        'sources': ['https://github.com/sigrokproject/libsigrok/blob/master/src/hardware/fx2lafw/protocol.c',
                    'https://sigrok.org/wiki/File_format:Sigrok/v2'],
    }
    BASE.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(serial_data.decode('utf-8', errors='replace'))
    print(json.dumps(report, indent=2))
    if not report['complete']:
        raise SystemExit(1)


if __name__ == '__main__':
    main()

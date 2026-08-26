#!/usr/bin/env python3
"""Program the UART flash writer, transfer U-Boot, then load the SoC bitstream."""

import argparse
import binascii
import glob
import os
from pathlib import Path
import select
import subprocess
import sys
import time

import serial


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vivado", default="vivado")
    parser.add_argument("--programmer-bit", required=True)
    parser.add_argument("--image")
    parser.add_argument("--soc-bit")
    parser.add_argument("--serial", default="auto")
    parser.add_argument("--data-bits", type=int, choices=(5, 8), default=5)
    parser.add_argument("--probe-seconds", type=float, default=10.0)
    parser.add_argument("--post-seconds", type=float, default=8.0)
    parser.add_argument("--probe-only", action="store_true")
    parser.add_argument("--transfer-only", action="store_true")
    parser.add_argument("--log-dir", required=True)
    return parser.parse_args()


def require_file(path, label):
    resolved = Path(path).resolve()
    if not resolved.is_file():
        raise SystemExit(f"{label} does not exist: {resolved}")
    return resolved


def serial_candidates(requested):
    if requested != "auto":
        return [Path(requested).resolve()]
    candidates = [Path(path) for path in sorted(glob.glob("/dev/serial/by-id/*"))]
    if not candidates:
        candidates = [Path(path) for path in sorted(glob.glob("/dev/ttyUSB*"))]
    if not candidates:
        raise SystemExit("no USB serial ports found")
    return candidates


def open_ports(paths, baudrate, data_bits, reset_input=True):
    ports = {}
    bytesize = serial.FIVEBITS if data_bits == 5 else serial.EIGHTBITS
    for path in paths:
        try:
            port = serial.Serial(
                str(path), baudrate=baudrate, bytesize=bytesize,
                parity=serial.PARITY_NONE, stopbits=1, timeout=0,
                write_timeout=2, exclusive=True,
            )
            if reset_input:
                port.reset_input_buffer()
            ports[path] = port
        except (OSError, serial.SerialException) as error:
            print(f"SERIAL_OPEN_FAILED={path}: {error}", file=sys.stderr)
    if not ports:
        raise SystemExit("failed to open every serial candidate")
    return ports


def program_bit(vivado, tcl_script, bit_file, log_file):
    env = os.environ.copy()
    env["BIT_FILE"] = str(bit_file)
    command = [
        vivado, "-mode", "batch", "-nolog", "-nojournal", "-notrace",
        "-source", str(tcl_script),
    ]
    result = subprocess.run(command, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log_file.write_text(result.stdout, encoding="utf-8", errors="replace")
    if result.returncode != 0 or "PROGRAM_RESULT=success" not in result.stdout:
        raise SystemExit(f"Vivado programming failed; see {log_file}")


def capture(ports, duration, logs):
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        ready, _, _ = select.select(list(ports.values()), [], [], 0.2)
        for port in ready:
            data = port.read(port.in_waiting or 1)
            if data:
                path = next(path for path, item in ports.items() if item is port)
                logs[path].extend(data)


def write_serial_logs(log_dir, prefix, logs):
    for path, data in logs.items():
        safe_name = path.name.replace("/", "_")
        (log_dir / f"{prefix}-{safe_name}.log").write_bytes(bytes(data))


def choose_active_port(logs, allow_silent=False):
    active = [path for path, data in logs.items() if data]
    if allow_silent and not active and len(logs) == 1:
        return next(iter(logs))
    if len(active) != 1:
        detail = ", ".join(f"{path}={len(logs[path])}B" for path in logs)
        raise SystemExit(f"expected one active programmer UART, found {len(active)}: {detail}")
    return active[0]


def read_port(port, duration):
    data = bytearray()
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        ready, _, _ = select.select([port], [], [], 0.2)
        if ready:
            chunk = port.read(port.in_waiting or 1)
            if chunk:
                data.extend(chunk)
    return bytes(data)


def wait_xmodem_response(port, timeout, raw_rx):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        ready, _, _ = select.select([port], [], [], 0.2)
        if not ready:
            continue
        data = port.read(port.in_waiting or 1)
        raw_rx.extend(data)
        for value in data:
            if value in (0x06, 0x15, 0x18):
                return value
    return None


def send_xmodem_crc(port, image, log_dir):
    data = image.read_bytes()
    raw_rx = bytearray()
    protocol_log = log_dir / "xmodem-protocol.log"
    with protocol_log.open("w", encoding="utf-8") as log:
        for offset in range(0, len(data), 128):
            block_index = offset // 128 + 1
            block_number = block_index & 0xff
            payload = data[offset:offset + 128].ljust(128, b"\x1a")
            crc = binascii.crc_hqx(payload, 0)
            packet = bytes((0x01, block_number, 0xff - block_number))
            packet += payload + crc.to_bytes(2, "big")

            for attempt in range(1, 11):
                port.write(packet)
                port.flush()
                response = wait_xmodem_response(port, 10.0, raw_rx)
                if response == 0x06:
                    break
                if response == 0x18:
                    raise SystemExit(f"XMODEM cancelled by receiver at block {block_index}")
                log.write(f"block={block_index} attempt={attempt} response={response}\n")
                log.flush()
            else:
                raise SystemExit(f"XMODEM block {block_index} was not acknowledged")

            if block_index == 1 or block_index % 128 == 0:
                log.write(f"block={block_index} acknowledged\n")
                log.flush()

        for attempt in range(1, 11):
            port.write(b"\x04")
            port.flush()
            response = wait_xmodem_response(port, 10.0, raw_rx)
            log.write(f"eot attempt={attempt} response={response}\n")
            log.flush()
            if response == 0x06:
                break
            if response == 0x18:
                raise SystemExit("XMODEM cancelled by receiver during EOT")
        else:
            raise SystemExit("XMODEM EOT was not acknowledged")

    (log_dir / "xmodem-receiver-raw.log").write_bytes(raw_rx)


def xmodem_send(port, image, log_dir):
    port.write(b"\r")
    port.flush()
    prompt = read_port(port, 2.0)
    (log_dir / "programmer-enter-prompt.log").write_bytes(prompt)
    if b"x" not in prompt.lower():
        raise SystemExit("programmer did not request x after Enter")

    port.write(b"x")
    port.flush()
    handshake = read_port(port, 4.0)
    (log_dir / "programmer-xmodem-handshake.log").write_bytes(handshake)
    if handshake.count(b"C") < 3:
        raise SystemExit("programmer did not return at least three XMODEM C bytes")

    send_xmodem_crc(port, image, log_dir)


def main():
    args = parse_args()
    tool_dir = Path(__file__).resolve().parent
    tcl_script = tool_dir / "cpustcpu_program_fpga.tcl"
    programmer_bit = require_file(args.programmer_bit, "programmer bitstream")
    image = require_file(args.image, "U-Boot image") if args.image else None
    soc_bit = require_file(args.soc_bit, "SoC bitstream") if args.soc_bit else None
    if not args.probe_only and image is None:
        raise SystemExit("--image is required outside --probe-only")
    if not args.probe_only and not args.transfer_only and soc_bit is None:
        raise SystemExit("--soc-bit is required outside --probe-only/--transfer-only")

    log_dir = Path(args.log_dir).resolve()
    log_dir.mkdir(parents=True, exist_ok=True)
    paths = serial_candidates(args.serial)
    ports = open_ports(paths, 230400, args.data_bits)
    pre_logs = {path: bytearray() for path in ports}
    try:
        program_bit(args.vivado, tcl_script, programmer_bit,
                    log_dir / "vivado-programmer.log")
        capture(ports, args.probe_seconds, pre_logs)
        write_serial_logs(log_dir, "programmer-probe", pre_logs)
        selected_path = choose_active_port(pre_logs, args.serial != "auto")
        print(f"PROGRAMMER_SERIAL={selected_path}")
        if args.probe_only:
            print("PROBE_RESULT=success")
            return

        selected = ports[selected_path]
        xmodem_send(selected, image, log_dir)
        post_logs = {path: bytearray() for path in ports}
        capture(ports, args.post_seconds, post_logs)
        write_serial_logs(log_dir, "programmer-post", post_logs)
    finally:
        for port in ports.values():
            port.close()

    if args.transfer_only:
        print("TRANSFER_RESULT=success")
        return

    program_bit(args.vivado, tcl_script, soc_bit, log_dir / "vivado-soc.log")
    print("FLASH_SEQUENCE_RESULT=success")


if __name__ == "__main__":
    main()

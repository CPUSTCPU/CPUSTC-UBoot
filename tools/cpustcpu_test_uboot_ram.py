#!/usr/bin/env python3
"""Load a U-Boot binary into DDR with XMODEM, verify it, and run it."""

import argparse
import binascii
from pathlib import Path
import select
import time

from cpustcpu_flash_uboot import open_ports, read_port, require_file, send_xmodem_crc


PROMPT = b"u-boot@LoongsonSoC# "
AUTOBOOT_PROMPT = b"Hit any key to stop autoboot:"


def boot_ram_image(port, address, timeout, stop_autoboot):
    data = bytearray()
    deadline = time.monotonic() + timeout
    interrupted = False

    port.write(f"go {address}\r".encode("ascii"))
    port.flush()
    while time.monotonic() < deadline:
        ready, _, _ = select.select([port], [], [], 0.2)
        if not ready:
            continue
        chunk = port.read(port.in_waiting or 1)
        if not chunk:
            continue
        data.extend(chunk)
        if stop_autoboot and not interrupted and AUTOBOOT_PROMPT in data:
            port.write(b"\r")
            port.flush()
            interrupted = True
        if interrupted and PROMPT in data:
            break

    return bytes(data), interrupted


def run_uboot_command(port, command, timeout):
    data = bytearray()
    deadline = time.monotonic() + timeout

    port.write(command.encode("ascii") + b"\r")
    port.flush()
    while time.monotonic() < deadline:
        ready, _, _ = select.select([port], [], [], 0.2)
        if not ready:
            continue
        chunk = port.read(port.in_waiting or 1)
        if not chunk:
            continue
        data.extend(chunk)
        if PROMPT in data:
            return bytes(data)

    return bytes(data)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--image", required=True)
    parser.add_argument("--serial", required=True)
    parser.add_argument("--address", default="a0200000")
    parser.add_argument("--log-dir", required=True)
    parser.add_argument(
        "--command", action="append", default=[],
        help="interrupt candidate autoboot and run this command; may be repeated",
    )
    parser.add_argument("--boot-timeout", type=float, default=15.0)
    parser.add_argument("--command-timeout", type=float, default=30.0)
    args = parser.parse_args()

    image = require_file(args.image, "U-Boot image")
    log_dir = Path(args.log_dir).resolve()
    log_dir.mkdir(parents=True, exist_ok=True)
    path = Path(args.serial).resolve()
    ports = open_ports([path], 115200, 8, reset_input=False)
    port = ports[path]

    try:
        port.write(b"\r")
        port.flush()
        prompt = read_port(port, 2.0)
        (log_dir / "uboot-prompt.log").write_bytes(prompt)
        if PROMPT not in prompt:
            raise SystemExit("working U-Boot prompt was not observed")

        port.write(f"loadx {args.address}\r".encode("ascii"))
        port.flush()
        handshake = read_port(port, 4.0)
        (log_dir / "loadx-handshake.log").write_bytes(handshake)
        if handshake.count(b"C") < 1:
            raise SystemExit("U-Boot loadx did not request XMODEM CRC mode")

        send_xmodem_crc(port, image, log_dir)
        loaded = read_port(port, 5.0)
        (log_dir / "loadx-complete.log").write_bytes(loaded)
        if PROMPT not in loaded:
            raise SystemExit("loadx did not return to the U-Boot prompt")

        expected_crc = binascii.crc32(image.read_bytes()) & 0xffffffff
        port.write(f"crc32 {args.address} {image.stat().st_size:x}\r".encode("ascii"))
        port.flush()
        crc_log = read_port(port, 3.0)
        (log_dir / "uboot-crc32.log").write_bytes(crc_log)
        if f"{expected_crc:08x}".encode("ascii") not in crc_log.lower():
            raise SystemExit(f"DDR CRC32 did not match {expected_crc:08x}")

        run_log, interrupted = boot_ram_image(
            port, args.address, args.boot_timeout, bool(args.command)
        )
        (log_dir / "ram-image-boot.log").write_bytes(run_log)
        if b"Cache init over" not in run_log:
            raise SystemExit("RAM image did not reach the early U-Boot serial output")
        if args.command and not interrupted:
            raise SystemExit("candidate U-Boot autoboot prompt was not observed")
        if args.command and PROMPT not in run_log:
            raise SystemExit("candidate U-Boot prompt was not observed after autoboot interrupt")

        for index, command in enumerate(args.command, start=1):
            try:
                command_log = run_uboot_command(
                    port, command, args.command_timeout
                )
            except UnicodeEncodeError as error:
                raise SystemExit("--command must contain only ASCII characters") from error
            command_log_path = log_dir / f"candidate-command-{index:02d}.log"
            command_log_path.write_bytes(command_log)
            if PROMPT not in command_log:
                raise SystemExit(
                    f"candidate command {index} timed out; see {command_log_path}"
                )
            print(f"CANDIDATE_COMMAND_{index}_LOG={command_log_path}")

        print(f"DDR_CRC32={expected_crc:08x}")
        print("RAM_IMAGE_BOOT_RESULT=success")
    finally:
        port.close()


if __name__ == "__main__":
    main()

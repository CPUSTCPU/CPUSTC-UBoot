#!/usr/bin/env python3
"""Load a file into U-Boot DDR with XMODEM and verify its CRC32."""

import argparse
import binascii
import hashlib
from pathlib import Path

from cpustcpu_flash_uboot import open_ports, read_port, require_file, send_xmodem_crc


PROMPT = b"u-boot@LoongsonSoC# "


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--image", required=True)
    parser.add_argument("--serial", required=True)
    parser.add_argument("--address", required=True)
    parser.add_argument("--pad-to", type=int, default=1)
    parser.add_argument("--pad-byte", type=lambda value: int(value, 16), default=0xff)
    parser.add_argument("--log-dir", required=True)
    args = parser.parse_args()

    image = require_file(args.image, "input image")
    if args.pad_to <= 0:
        raise SystemExit("--pad-to must be positive")
    if not 0 <= args.pad_byte <= 0xff:
        raise SystemExit("--pad-byte must fit in one byte")

    log_dir = Path(args.log_dir).resolve()
    log_dir.mkdir(parents=True, exist_ok=True)
    source = image.read_bytes()
    padded_size = ((len(source) + args.pad_to - 1) // args.pad_to) * args.pad_to
    payload = source.ljust(padded_size, bytes((args.pad_byte,)))
    transfer_image = log_dir / "transfer-image.bin"
    transfer_image.write_bytes(payload)
    crc = binascii.crc32(payload) & 0xffffffff
    (log_dir / "image-metadata.txt").write_text(
        f"source={image}\n"
        f"source_size={len(source)}\n"
        f"transfer_size={len(payload)}\n"
        f"pad_to={args.pad_to}\n"
        f"pad_byte=0x{args.pad_byte:02x}\n"
        f"source_sha256={hashlib.sha256(source).hexdigest()}\n"
        f"transfer_sha256={hashlib.sha256(payload).hexdigest()}\n"
        f"transfer_crc32={crc:08x}\n",
        encoding="utf-8",
    )

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

        send_xmodem_crc(port, transfer_image, log_dir)
        complete = read_port(port, 5.0)
        (log_dir / "loadx-complete.log").write_bytes(complete)
        if PROMPT not in complete:
            raise SystemExit("loadx did not return to the U-Boot prompt")

        port.write(f"crc32 {args.address} {len(payload):x}\r".encode("ascii"))
        port.flush()
        crc_log = read_port(port, 3.0)
        (log_dir / "ddr-crc32.log").write_bytes(crc_log)
        if f"{crc:08x}".encode("ascii") not in crc_log.lower():
            raise SystemExit(f"DDR CRC32 did not match {crc:08x}")
    finally:
        port.close()

    print(f"SOURCE_SIZE={len(source)}")
    print(f"TRANSFER_SIZE={len(payload)}")
    print(f"DDR_CRC32={crc:08x}")
    print("LOAD_FILE_RESULT=success")


if __name__ == "__main__":
    main()

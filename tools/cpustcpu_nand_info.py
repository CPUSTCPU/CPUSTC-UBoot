#!/usr/bin/env python3
"""Program the SoC bitstream, stop autoboot, and run the read-only NAND probe."""

import argparse
from pathlib import Path
import select
import time

from cpustcpu_flash_uboot import (
    open_ports,
    program_bit,
    require_file,
    serial_candidates,
    write_serial_logs,
)


PROMPT = b"u-boot@LoongsonSoC# "


def capture(ports, duration, logs, interrupt=False):
    deadline = time.monotonic() + duration
    interrupted = set()
    while time.monotonic() < deadline:
        ready, _, _ = select.select(list(ports.values()), [], [], 0.2)
        for port in ready:
            data = port.read(port.in_waiting or 1)
            if not data:
                continue
            path = next(path for path, item in ports.items() if item is port)
            logs[path].extend(data)
            if interrupt and path not in interrupted:
                port.write(b" ")
                port.flush()
                interrupted.add(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vivado", default="vivado")
    parser.add_argument("--soc-bit", required=True)
    parser.add_argument("--serial", default="auto")
    parser.add_argument("--data-bits", type=int, choices=(5, 8), default=8)
    parser.add_argument("--log-dir", required=True)
    args = parser.parse_args()

    soc_bit = require_file(args.soc_bit, "SoC bitstream")
    log_dir = Path(args.log_dir).resolve()
    log_dir.mkdir(parents=True, exist_ok=True)
    tcl_script = Path(__file__).resolve().parent / "cpustcpu_program_fpga.tcl"
    paths = serial_candidates(args.serial)
    program_bit(args.vivado, tcl_script, soc_bit, log_dir / "vivado-soc.log")
    ports = open_ports(paths, 115200, args.data_bits, reset_input=False)
    try:
        boot_logs = {path: bytearray() for path in ports}
        if args.serial != "auto" and len(ports) == 1:
            selected = next(iter(ports.values()))
            selected.write(b"\r")
            selected.flush()
        capture(ports, 12.0, boot_logs, interrupt=True)
        write_serial_logs(log_dir, "uboot-boot", boot_logs)
        active = [path for path, data in boot_logs.items() if data]
        if len(active) != 1:
            detail = ", ".join(f"{path}={len(boot_logs[path])}B" for path in boot_logs)
            raise SystemExit(f"expected one active U-Boot UART, found {len(active)}: {detail}")

        selected_path = active[0]
        if PROMPT not in boot_logs[selected_path]:
            raise SystemExit(f"U-Boot prompt was not observed on {selected_path}")

        selected = ports[selected_path]
        selected.write(b"nand info\r")
        selected.flush()
        command_logs = {path: bytearray() for path in ports}
        capture(ports, 5.0, command_logs)
        write_serial_logs(log_dir, "uboot-nand-info", command_logs)
        if PROMPT not in command_logs[selected_path]:
            raise SystemExit("nand info did not return to the U-Boot prompt")
        print(f"UBOOT_SERIAL={selected_path}")
        print("NAND_INFO_RESULT=completed")
    finally:
        for port in ports.values():
            port.close()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Passively measure CPUSTC Linux boot until a serial shell executes a command."""

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import secrets
import select
import sys
import time


HANDOFF_MARKER = b"do_bootelf_exec..."
INIT_EXEC_PATTERN = re.compile(
    rb"\[\s*([0-9]+(?:\.[0-9]+)?)\]\s+Run (/\S+) as init process\r?\n"
)
USERSPACE_READY_PATTERN = re.compile(
    rb"CPUSTC_USERSPACE_INIT_DONE uptime_s=([0-9]+(?:\.[0-9]+)?)\r?\n"
)
LOGIN_PATTERN = re.compile(rb"(?:^|[\r\n])([^\r\n]* login:\s*)")
PASSWORD_PATTERN = re.compile(rb"Password:\s*")
SHELL_PROMPT_PATTERN = re.compile(rb"(?:^|[\r\n])[^\r\n]*#\s+")
LOGIN_FAILED_PATTERN = re.compile(rb"Login incorrect")


def parse_args():
    parser = argparse.ArgumentParser(
        description=(
            "Wait for a normal CPUSTC boot, timestamp the U-Boot/Linux markers, "
            "then log in and prove that the serial shell executes a command."
        )
    )
    parser.add_argument("--serial", required=True, help="serial device path")
    parser.add_argument(
        "--output-dir",
        required=True,
        help="new directory for the raw serial log and timing report",
    )
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--username", default="root")
    parser.add_argument(
        "--password",
        default=os.environ.get("CPUSTC_BOOT_PASSWORD"),
        help="login password (or set CPUSTC_BOOT_PASSWORD)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=1200.0,
        help="seconds to wait for the shell-ready result (default: 1200)",
    )
    parser.add_argument(
        "--send-byte-delay",
        type=float,
        default=0.0,
        help=(
            "delay in seconds between serial login/probe bytes to avoid target "
            "UART overruns (default: 0)"
        ),
    )
    parser.add_argument(
        "--interaction-settle-delay",
        type=float,
        default=0.0,
        help="delay after each detected login/shell prompt before sending (default: 0)",
    )
    parser.add_argument(
        "--show-serial", action="store_true", help="mirror serial data to stdout"
    )
    args = parser.parse_args()
    if args.baudrate <= 0:
        parser.error("--baudrate must be positive")
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    if args.send_byte_delay < 0:
        parser.error("--send-byte-delay must be non-negative")
    if args.interaction_settle_delay < 0:
        parser.error("--interaction-settle-delay must be non-negative")
    if args.password is None:
        parser.error("--password or CPUSTC_BOOT_PASSWORD is required")
    try:
        args.username.encode("ascii")
        args.password.encode("ascii")
    except UnicodeEncodeError:
        parser.error("--username and --password must contain only ASCII characters")
    return args


class MarkerParser:
    def __init__(self, capture_start_ns, shell_marker):
        self.capture_start_ns = capture_start_ns
        self.shell_marker = shell_marker
        self.shell_ready_pattern = re.compile(
            rb"(?:^|[\r\n])"
            + re.escape(shell_marker)
            + rb" uptime_s=([0-9]+(?:\.[0-9]+)?)\r?\n"
        )
        self.data = bytearray()
        self.events = {}

    def record(self, name, host_ns, stream_offset, evidence, **values):
        if name in self.events:
            return False
        event = {
            "host_monotonic_ns": host_ns,
            "host_since_capture_s": round(
                (host_ns - self.capture_start_ns) / 1_000_000_000, 9
            ),
            "stream_offset": stream_offset,
            "evidence": evidence,
        }
        event.update(values)
        self.events[name] = event
        return True

    def feed(self, chunk, host_ns):
        self.data.extend(chunk)
        data = bytes(self.data)

        if "kernel_handoff" not in self.events:
            offset = data.find(HANDOFF_MARKER)
            if offset >= 0:
                self.record(
                    "kernel_handoff",
                    host_ns,
                    offset + len(HANDOFF_MARKER),
                    HANDOFF_MARKER.decode("ascii"),
                )

        handoff = self.events.get("kernel_handoff")
        if handoff is None:
            return
        start = handoff["stream_offset"]

        self._record_pattern(
            "init_exec",
            INIT_EXEC_PATTERN,
            data,
            start,
            host_ns,
            target_value_name="kernel_timestamp_s",
        )
        self._record_pattern(
            "userspace_init_done",
            USERSPACE_READY_PATTERN,
            data,
            start,
            host_ns,
            target_value_name="target_uptime_s",
        )
        self._record_pattern("login_prompt", LOGIN_PATTERN, data, start, host_ns)
        self._record_pattern(
            "shell_ready",
            self.shell_ready_pattern,
            data,
            start,
            host_ns,
            target_value_name="target_uptime_s",
        )

    def _record_pattern(
        self,
        name,
        pattern,
        data,
        start,
        host_ns,
        target_value_name=None,
    ):
        if name in self.events:
            return
        match = pattern.search(data, start)
        if match is None:
            return
        values = {}
        if target_value_name is not None:
            values[target_value_name] = float(match.group(1))
        evidence = match.group(0).decode("ascii", errors="replace").strip()
        self.record(name, host_ns, match.end(), evidence, **values)

    def search_after(self, pattern, offset):
        return pattern.search(bytes(self.data), offset)


def build_shell_probe(shell_marker):
    marker = shell_marker.decode("ascii")
    split = len(marker) // 2
    first = marker[:split]
    second = marker[split:]
    command = (
        "read u _</proc/uptime;echo '"
        + first
        + "''"
        + second
        + "' uptime_s=$u"
    )
    encoded = command.encode("ascii")
    if shell_marker in encoded:
        raise AssertionError("shell marker must not appear literally in the echoed command")
    return encoded


def duration_between(events, start, end):
    if start not in events or end not in events:
        return None
    return round(
        (
            events[end]["host_monotonic_ns"]
            - events[start]["host_monotonic_ns"]
        )
        / 1_000_000_000,
        9,
    )


def compute_durations(events):
    durations = {}
    external_pairs = (
        ("linux_to_init_exec_external_s", "kernel_handoff", "init_exec"),
        (
            "linux_to_userspace_init_done_external_s",
            "kernel_handoff",
            "userspace_init_done",
        ),
        ("linux_to_login_prompt_external_s", "kernel_handoff", "login_prompt"),
        ("linux_to_shell_ready_external_s", "kernel_handoff", "shell_ready"),
    )
    for name, start, end in external_pairs:
        value = duration_between(events, start, end)
        if value is not None:
            durations[name] = value

    if "init_exec" in events:
        durations["kernel_to_init_exec_internal_s"] = events["init_exec"][
            "kernel_timestamp_s"
        ]
    if "userspace_init_done" in events:
        durations["kernel_to_userspace_init_done_internal_s"] = events[
            "userspace_init_done"
        ]["target_uptime_s"]
    if "shell_ready" in events:
        durations["kernel_to_shell_ready_internal_s"] = events["shell_ready"][
            "target_uptime_s"
        ]
    if "userspace_init_done" in events and "shell_ready" in events:
        durations["userspace_init_done_to_shell_ready_internal_s"] = round(
            events["shell_ready"]["target_uptime_s"]
            - events["userspace_init_done"]["target_uptime_s"],
            9,
        )
    return durations


def write_report(path, args, parser, status, started_utc, error=None):
    report = {
        "schema_version": 1,
        "status": status,
        "capture_started_utc": started_utc,
        "serial_requested": args.serial,
        "serial_resolved": str(Path(args.serial).resolve()),
        "baudrate": args.baudrate,
        "timeout_s": args.timeout,
        "send_byte_delay_s": args.send_byte_delay,
        "interaction_settle_delay_s": args.interaction_settle_delay,
        "username": args.username,
        "events": parser.events,
        "durations": compute_durations(parser.events),
        "measurement_scope": (
            "U-Boot kernel handoff to successful Linux serial-shell command"
        ),
    }
    if error is not None:
        report["error"] = str(error)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    temporary.replace(path)


def send_line(port, value, byte_delay_s=0.0):
    line = value + b"\r"
    if byte_delay_s == 0:
        port.write(line)
    else:
        for index, byte in enumerate(line):
            port.write(bytes((byte,)))
            if index + 1 < len(line):
                time.sleep(byte_delay_s)
    port.flush()


def run_measurement(args, output_dir):
    try:
        import serial
    except ImportError as error:
        raise RuntimeError("pyserial is required: install the python3-serial package") from error

    capture_start_ns = time.monotonic_ns()
    started_utc = datetime.now(timezone.utc).isoformat()
    shell_marker = ("CPUSTC_SHELL_READY_" + secrets.token_hex(6)).encode("ascii")
    parser = MarkerParser(capture_start_ns, shell_marker)
    report_path = output_dir / "linux-boot-timing.json"
    raw_path = output_dir / "serial.raw.log"
    chunks_path = output_dir / "serial-chunks.csv"
    write_report(report_path, args, parser, "starting", started_utc)

    port = None
    status = "failed"
    error = None
    interaction_offset = 0
    username_sent = False
    password_sent = False
    shell_probe_sent = False

    try:
        port = serial.Serial(
            args.serial,
            baudrate=args.baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0,
            write_timeout=2,
            exclusive=True,
        )
        port.reset_input_buffer()
        print(
            f"SERIAL_READY={args.serial} baudrate={args.baudrate}",
            flush=True,
        )
        write_report(report_path, args, parser, "capturing", started_utc)

        with raw_path.open("xb") as raw_log, chunks_path.open(
            "x", encoding="ascii", newline=""
        ) as chunk_log:
            chunk_log.write("host_monotonic_ns,stream_offset,length\n")
            deadline_ns = capture_start_ns + int(args.timeout * 1_000_000_000)

            while time.monotonic_ns() < deadline_ns:
                ready, _, _ = select.select([port], [], [], 0.2)
                if not ready:
                    continue
                chunk = port.read(port.in_waiting or 1)
                if not chunk:
                    continue

                received_ns = time.monotonic_ns()
                offset = raw_log.tell()
                raw_log.write(chunk)
                chunk_log.write(f"{received_ns},{offset},{len(chunk)}\n")
                if args.show_serial:
                    sys.stdout.buffer.write(chunk)
                    sys.stdout.buffer.flush()

                previous_event_count = len(parser.events)
                parser.feed(chunk, received_ns)
                if len(parser.events) != previous_event_count:
                    raw_log.flush()
                    chunk_log.flush()
                    write_report(report_path, args, parser, "capturing", started_utc)

                if "kernel_handoff" not in parser.events:
                    continue

                if "login_prompt" in parser.events and not username_sent:
                    raw_log.flush()
                    chunk_log.flush()
                    time.sleep(args.interaction_settle_delay)
                    send_line(
                        port,
                        args.username.encode("ascii"),
                        args.send_byte_delay,
                    )
                    username_sent = True
                    interaction_offset = len(parser.data)
                    parser.record(
                        "username_sent",
                        time.monotonic_ns(),
                        interaction_offset,
                        args.username,
                    )
                    write_report(
                        report_path, args, parser, "capturing", started_utc
                    )
                    continue

                if username_sent and not password_sent and not shell_probe_sent:
                    if parser.search_after(LOGIN_FAILED_PATTERN, interaction_offset):
                        raise RuntimeError("serial login was rejected")
                    password_prompt = parser.search_after(
                        PASSWORD_PATTERN, interaction_offset
                    )
                    shell_prompt = parser.search_after(
                        SHELL_PROMPT_PATTERN, interaction_offset
                    )
                    if password_prompt is not None:
                        raw_log.flush()
                        chunk_log.flush()
                        time.sleep(args.interaction_settle_delay)
                        send_line(
                            port,
                            args.password.encode("ascii"),
                            args.send_byte_delay,
                        )
                        password_sent = True
                        interaction_offset = len(parser.data)
                        parser.record(
                            "password_sent",
                            time.monotonic_ns(),
                            interaction_offset,
                            "password omitted",
                        )
                        write_report(
                            report_path, args, parser, "capturing", started_utc
                        )
                        continue
                    if shell_prompt is not None:
                        raw_log.flush()
                        chunk_log.flush()
                        time.sleep(args.interaction_settle_delay)
                        send_line(
                            port,
                            build_shell_probe(shell_marker),
                            args.send_byte_delay,
                        )
                        shell_probe_sent = True
                        interaction_offset = len(parser.data)
                        parser.record(
                            "shell_probe_sent",
                            time.monotonic_ns(),
                            interaction_offset,
                            "unique shell probe",
                        )
                        write_report(
                            report_path, args, parser, "capturing", started_utc
                        )
                        continue

                if password_sent and not shell_probe_sent:
                    if parser.search_after(LOGIN_FAILED_PATTERN, interaction_offset):
                        raise RuntimeError("serial login was rejected")
                    shell_prompt = parser.search_after(
                        SHELL_PROMPT_PATTERN, interaction_offset
                    )
                    if shell_prompt is not None:
                        raw_log.flush()
                        chunk_log.flush()
                        time.sleep(args.interaction_settle_delay)
                        send_line(
                            port,
                            build_shell_probe(shell_marker),
                            args.send_byte_delay,
                        )
                        shell_probe_sent = True
                        interaction_offset = len(parser.data)
                        parser.record(
                            "shell_probe_sent",
                            time.monotonic_ns(),
                            interaction_offset,
                            "unique shell probe",
                        )
                        write_report(
                            report_path, args, parser, "capturing", started_utc
                        )
                        continue

                if "shell_ready" in parser.events:
                    if "userspace_init_done" in parser.events:
                        shell_uptime = parser.events["shell_ready"]["target_uptime_s"]
                        userspace_uptime = parser.events["userspace_init_done"][
                            "target_uptime_s"
                        ]
                        if shell_uptime < userspace_uptime:
                            raise RuntimeError(
                                "shell-ready uptime predates userspace-init uptime"
                            )
                    status = "success"
                    break
            else:
                raise RuntimeError(
                    f"timed out after {args.timeout:g}s before the shell-ready result"
                )

        if status != "success":
            raise RuntimeError("capture ended without the shell-ready result")
    except KeyboardInterrupt:
        error = RuntimeError("measurement interrupted")
    except (OSError, RuntimeError) as caught:
        error = caught
    finally:
        if port is not None:
            port.close()
        write_report(report_path, args, parser, status, started_utc, error)

    if error is not None:
        raise RuntimeError(f"{error}; partial evidence: {output_dir}")
    return report_path, compute_durations(parser.events)


def main():
    args = parse_args()
    output_dir = Path(args.output_dir).resolve()
    try:
        output_dir.mkdir(parents=True, exist_ok=False)
    except FileExistsError:
        raise SystemExit(f"output directory already exists: {output_dir}")

    try:
        report_path, durations = run_measurement(args, output_dir)
    except (OSError, RuntimeError) as error:
        raise SystemExit(str(error)) from error

    print("RESULT=success", flush=True)
    print(f"REPORT={report_path}", flush=True)
    for name, value in sorted(durations.items()):
        print(f"{name.upper()}={value:.9f}", flush=True)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3

import time
import unittest
from unittest import mock

import cpustcpu_measure_linux_boot as measure


class MarkerParserTest(unittest.TestCase):
    def setUp(self):
        self.start_ns = time.monotonic_ns()
        self.shell_marker = b"CPUSTC_SHELL_READY_0123456789ab"
        self.parser = measure.MarkerParser(self.start_ns, self.shell_marker)

    def feed(self, data, elapsed_s):
        self.parser.feed(
            data, self.start_ns + int(elapsed_s * 1_000_000_000)
        )

    def test_fragmented_boot_markers_and_durations(self):
        self.feed(b"do_bootelf_", 0.1)
        self.assertEqual({}, self.parser.events)

        self.feed(b"exec...\r\n[    1.250000] Run /init as init process\r\n", 0.2)
        self.feed(b"CPUSTC_USERSPACE_INIT_DONE uptime_s=4.50\r\n", 4.7)
        self.feed(b"cpustcos login: ", 4.8)
        self.feed(b"\r\n" + self.shell_marker[:12], 5.0)
        self.feed(self.shell_marker[12:] + b" uptime_s=5.25\r\n", 5.3)

        events = self.parser.events
        self.assertEqual(1.25, events["init_exec"]["kernel_timestamp_s"])
        self.assertEqual(4.5, events["userspace_init_done"]["target_uptime_s"])
        self.assertEqual(5.25, events["shell_ready"]["target_uptime_s"])

        durations = measure.compute_durations(events)
        self.assertEqual(5.1, durations["linux_to_shell_ready_external_s"])
        self.assertEqual(5.25, durations["kernel_to_shell_ready_internal_s"])
        self.assertEqual(
            0.75, durations["userspace_init_done_to_shell_ready_internal_s"]
        )

    def test_ignores_linux_markers_before_handoff(self):
        self.feed(b"[ 9.0] Run /init as init process\r\ncpustcos login: ", 0.1)
        self.assertEqual({}, self.parser.events)

    def test_accepts_login_prompt_followed_by_kernel_output(self):
        self.feed(b"do_bootelf_exec...\r\n", 0.1)
        self.feed(
            b"\rcpustcos login: [ 137.536000] deferred kernel message\r\n",
            1.0,
        )
        self.assertEqual(
            "cpustcos login:", self.parser.events["login_prompt"]["evidence"]
        )

    def test_shell_probe_does_not_echo_the_complete_result_marker(self):
        command = measure.build_shell_probe(self.shell_marker)
        self.assertNotIn(self.shell_marker, command)
        self.assertIn(b"/proc/uptime", command)

    def test_waits_for_newline_before_accepting_fragmented_uptime(self):
        self.feed(b"do_bootelf_exec...\r\n", 0.1)
        self.feed(b"CPUSTC_USERSPACE_INIT_DONE uptime_s=17", 17.0)
        self.assertNotIn("userspace_init_done", self.parser.events)
        self.feed(b"6.03\r\n", 17.1)
        self.assertEqual(
            176.03,
            self.parser.events["userspace_init_done"]["target_uptime_s"],
        )

        self.feed(b"\r\n" + self.shell_marker + b" uptime_s=1", 19.0)
        self.assertNotIn("shell_ready", self.parser.events)
        self.feed(b"91.14\r\n", 19.1)
        self.assertEqual(
            191.14,
            self.parser.events["shell_ready"]["target_uptime_s"],
        )


class SendLineTest(unittest.TestCase):
    def test_paces_each_byte_and_flushes(self):
        port = mock.Mock()

        with mock.patch.object(measure.time, "sleep") as sleep:
            measure.send_line(port, b"ab", byte_delay_s=0.02)

        self.assertEqual(
            [mock.call(b"a"), mock.call(b"b"), mock.call(b"\r")],
            port.write.call_args_list,
        )
        self.assertEqual([mock.call(0.02), mock.call(0.02)], sleep.call_args_list)
        port.flush.assert_called_once_with()

    def test_zero_delay_keeps_single_write(self):
        port = mock.Mock()

        measure.send_line(port, b"ab", byte_delay_s=0)

        port.write.assert_called_once_with(b"ab\r")
        port.flush.assert_called_once_with()


if __name__ == "__main__":
    unittest.main()

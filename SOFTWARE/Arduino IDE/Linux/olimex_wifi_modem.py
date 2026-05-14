#!/usr/bin/env python3
"""Linux driver for MOD-ESP32-C5 running the Olimex WiFi modem firmware."""

import time

import serial


class OlimexWiFiModem:
    def __init__(self, port="/dev/ttyS4", baudrate=115200, debug=True):
        self.ser = serial.Serial(port=port, baudrate=baudrate, timeout=0.05)
        self.debug = debug
        self.pending_clients = []
        self.line_fragment = ""

    def close(self):
        self.ser.close()

    def flush_input(self):
        self.ser.reset_input_buffer()

    def send_cmd(self, cmd):
        if self.debug:
            print(f"[TX] {cmd}")
        self.ser.write((cmd + "\n").encode("utf-8"))
        self.ser.flush()

    def normalize_line(self, line):
        known = (
            "PONG",
            "WIFI:CONNECTED",
            "IP:",
            "SERVER:STARTED",
            "SERVER:CLIENT:",
            "DATA:",
            "READY",
            "OK",
            "END",
            "ERROR:",
        )

        if self.line_fragment:
            line = self.line_fragment + line
            self.line_fragment = ""

        if line in ("WIFI:", "IP:", "DATA:", "SERVER:CLIENT:", "ERROR:"):
            self.line_fragment = line
            return ""

        if any(prefix.startswith(line) and prefix != line for prefix in known):
            self.line_fragment = line
            return ""

        return line

    def read_line(self, timeout=2.0):
        end = time.monotonic() + timeout
        data = bytearray()

        while time.monotonic() < end:
            b = self.ser.read(1)
            if not b:
                continue

            ch = b[0]
            if ch == ord("\r"):
                continue

            if ch == ord("\n"):
                line = data.decode("utf-8", errors="replace").strip()
                if line.startswith(">"):
                    line = line[1:].strip()
                if line:
                    return self.normalize_line(line)
                data.clear()
                continue

            if ch == ord(">") and not data:
                continue

            data.append(ch)

        line = data.decode("utf-8", errors="replace").strip()
        if line.startswith(">"):
            line = line[1:].strip()
        if line:
            return self.normalize_line(line)
        return line

    def read_exact(self, size, timeout=2.0):
        end = time.monotonic() + timeout
        data = bytearray()

        while len(data) < size and time.monotonic() < end:
            chunk = self.ser.read(size - len(data))
            if chunk:
                data.extend(chunk)

        if len(data) != size:
            raise TimeoutError(f"Expected {size} bytes, received {len(data)}")

        return bytes(data)

    def cache_client_line(self, line):
        if line.startswith("SERVER:CLIENT:"):
            self.pending_clients.append(int(line.split(":", 2)[2]))
            return True
        return False

    def wait_for(self, token, timeout=3.0):
        end = time.monotonic() + timeout
        recent = ""

        while time.monotonic() < end:
            line = self.read_line(timeout=0.2)
            if not line:
                continue

            if self.debug:
                print(f"[RX] {line}")

            self.cache_client_line(line)

            if line.startswith("ERROR:"):
                return False

            recent = (recent + line)[-128:]

            if token in line or token in recent:
                return True

        return False

    def begin(self):
        self.flush_input()
        self.send_cmd("ping")
        return self.wait_for("PONG", timeout=3.0)

    def connect_wifi(self, ssid, password):
        self.flush_input()
        self.send_cmd(f"connect {ssid} {password}")
        return self.wait_for("WIFI:CONNECTED", timeout=15.0)

    def ip(self):
        self.flush_input()
        self.send_cmd("ip")

        end = time.monotonic() + 3.0
        while time.monotonic() < end:
            line = self.read_line(timeout=0.5)
            if not line:
                continue

            if self.debug:
                print(f"[RX] {line}")

            self.cache_client_line(line)

            if line.startswith("IP:"):
                return line[3:]

        return ""

    def server_start(self, port=80):
        self.flush_input()
        self.send_cmd(f"server start {port}")
        return self.wait_for("SERVER:STARTED", timeout=3.0)

    def server_available(self):
        if self.pending_clients:
            return self.pending_clients.pop(0)

        line = self.read_line(timeout=0.05)
        if not line:
            return -1

        if self.debug:
            print(f"[RX] {line}")

        if line.startswith("SERVER:CLIENT:"):
            return int(line.split(":", 2)[2])

        return -1

    def server_read(self, client_id, timeout=2.0):
        self.send_cmd(f"server read {client_id}")
        end = time.monotonic() + timeout

        while time.monotonic() < end:
            line = self.read_line(timeout=0.2)
            if not line:
                continue

            if self.debug:
                print(f"[RX] {line}")

            if line in ("ERROR:BAD_ID", "ERROR:CLOSED"):
                return None

            if line.startswith("ERROR:"):
                raise RuntimeError(line)

            if self.cache_client_line(line):
                continue

            if not line.startswith("DATA:"):
                continue

            size = int(line[5:])
            payload = self.read_exact(size, timeout=timeout)

            trailer_end = time.monotonic() + 0.5
            while time.monotonic() < trailer_end:
                trailer = self.read_line(timeout=0.1)
                if trailer == "END":
                    break
                if trailer:
                    if self.debug:
                        print(f"[RX] {trailer}")
                    self.cache_client_line(trailer)

            return payload

        raise TimeoutError("Timed out waiting for DATA")

    def server_write(self, client_id, data):
        if isinstance(data, str):
            data = data.encode("utf-8")

        self.send_cmd(f"server write {client_id} {len(data)}")

        if not self.wait_for("READY", timeout=3.0):
            return False

        self.ser.write(data)
        self.ser.flush()

        return self.wait_for("OK", timeout=3.0)

    def server_close(self, client_id):
        self.send_cmd(f"server close {client_id}")
        return self.wait_for("OK", timeout=3.0)

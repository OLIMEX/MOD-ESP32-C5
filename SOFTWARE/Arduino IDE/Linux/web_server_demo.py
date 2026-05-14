#!/usr/bin/env python3

import argparse
from pathlib import Path
import time

from olimex_wifi_modem import OlimexWiFiModem


class LinuxLed:
    def __init__(self, led_name=None):
        self.path = self.find_led(led_name)
        self.enabled = self.path is not None

        if not self.enabled:
            print("No Linux LED found; using software state only.")
            return

        self.trigger = self.path / "trigger"
        self.brightness = self.path / "brightness"
        self.max_brightness = self.path / "max_brightness"

        if self.trigger.exists():
            self.trigger.write_text("none")

        print(f"Using Linux LED: {self.path.name}")

    @staticmethod
    def available_leds():
        root = Path("/sys/class/leds")
        if not root.exists():
            return []
        return sorted(p for p in root.iterdir() if p.is_dir())

    @classmethod
    def find_led(cls, led_name):
        leds = cls.available_leds()

        if led_name:
            selected = Path(led_name)
            if selected.exists():
                return selected

            for led in leds:
                if led.name == led_name:
                    return led

            print(f"Requested LED '{led_name}' was not found.")

        for text in ("led1", "green", "usr", "user"):
            for led in leds:
                if text in led.name.lower():
                    return led

        if leds:
            return leds[0]

        return None

    def write(self, enabled):
        if not self.enabled:
            return

        max_value = "1"
        if self.max_brightness.exists():
            max_value = self.max_brightness.read_text().strip() or "1"

        self.brightness.write_text(max_value if enabled else "0")


def http_response(body, content_type="text/html", status="200 OK"):
    body_bytes = body.encode("utf-8")
    headers = (
        f"HTTP/1.1 {status}\r\n"
        f"Content-Type: {content_type}\r\n"
        f"Content-Length: {len(body_bytes)}\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-store\r\n"
        "\r\n"
    ).encode("utf-8")
    return headers + body_bytes


def no_content_response():
    return (
        "HTTP/1.1 204 No Content\r\n"
        "Connection: close\r\n"
        "Cache-Control: no-store\r\n"
        "\r\n"
    ).encode("utf-8")


def send_response(modem, client_id, response):
    if modem.server_write(client_id, response):
        return

    print("Client closed before response was written")


def page(led_state):
    state = "ON" if led_state else "OFF"
    return f"""<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>OLinuXino + MOD-ESP32-C5</title>
</head>
<body style="font-family:Arial;text-align:center;margin-top:40px;">
<h1>OLinuXino + MOD-ESP32-C5</h1>
<h2>LED1 is <span id="state">{state}</span></h2>
<button id="toggle" onclick="toggleLed()" style="width:220px;height:80px;font-size:28px;">
Toggle
</button>
<script>
function toggleLed(){{
  const button = document.getElementById('toggle');
  const state = document.getElementById('state');
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 2500);
  button.disabled = true;
  fetch('/toggle', {{cache: 'no-store', signal: controller.signal}})
    .then(response => response.text())
    .then(text => {{ state.textContent = text; }})
    .catch(() => {{ state.textContent = 'RETRY'; }})
    .finally(() => {{ clearTimeout(timeout); button.disabled = false; }});
}}
</script>
</body>
</html>"""


def request_path(request_bytes):
    lines = request_bytes.decode("iso-8859-1", errors="replace").splitlines()
    if not lines:
        return None

    parts = lines[0].split()
    if len(parts) >= 2:
        return parts[1]

    return None


def handle_client(modem, client_id, led, led_state):
    request = modem.server_read(client_id)

    if request is None:
        print("Stale or closed client, skipping")
        return led_state

    path = request_path(request)

    if path is None:
        print("Empty HTTP request, closing client")
        return led_state

    print(f"Request path: {path}")

    if path == "/favicon.ico":
        send_response(modem, client_id, no_content_response())
        return led_state

    if path == "/toggle":
        led_state = not led_state
        if led is not None:
            led.write(led_state)
        state = "ON" if led_state else "OFF"
        print(f"LED1 state changed to: {state}")
        send_response(modem, client_id, http_response(state, "text/plain"))
        return led_state

    send_response(modem, client_id, http_response(page(led_state)))
    return led_state


def main():
    parser = argparse.ArgumentParser(description="MOD-ESP32-C5 Linux web demo")
    parser.add_argument("--port", default="/dev/ttyS4", help="UEXT UART device")
    parser.add_argument("--ssid", required=True, help="WiFi network name")
    parser.add_argument("--password", required=True, help="WiFi password")
    parser.add_argument("--http-port", type=int, default=80, help="HTTP port")
    parser.add_argument("--led", help="Linux LED name or /sys/class/leds path")
    parser.add_argument("--no-led", action="store_true", help="Use software state only")
    args = parser.parse_args()

    modem = OlimexWiFiModem(port=args.port, debug=True)
    led = None if args.no_led else LinuxLed(args.led)
    if led is not None and not led.enabled:
        led = None

    led_state = False
    if led is not None:
        led.write(led_state)

    try:
        print("================================")
        print("MOD-ESP32-C5 Linux Web Demo")
        print("================================")
        print(f"Serial port: {args.port}")
        print("Checking modem...")

        if not modem.begin():
            raise RuntimeError("Modem not detected")

        print("Modem OK")
        print("Connecting to WiFi...")

        if not modem.connect_wifi(args.ssid, args.password):
            raise RuntimeError("WiFi failed")

        ip = modem.ip()
        print(f"WiFi connected, IP: {ip}")

        if not modem.server_start(args.http_port):
            raise RuntimeError("HTTP server failed")

        print("HTTP server started")
        if args.http_port == 80:
            print(f"Open browser: http://{ip}")
        else:
            print(f"Open browser: http://{ip}:{args.http_port}")

        while True:
            client_id = modem.server_available()
            if client_id < 0:
                time.sleep(0.01)
                continue

            print(f"Client connected, ID={client_id}")

            try:
                led_state = handle_client(modem, client_id, led, led_state)

            except (RuntimeError, TimeoutError, OSError) as exc:
                print(f"Client handling error: {exc}")

            finally:
                modem.server_close(client_id)
                print("Client closed")

    finally:
        if led is not None:
            led.write(False)
        modem.close()


if __name__ == "__main__":
    main()

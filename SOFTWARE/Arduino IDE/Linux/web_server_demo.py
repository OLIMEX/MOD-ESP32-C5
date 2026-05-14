#!/usr/bin/env python3

import argparse
import time

from olimex_wifi_modem import OlimexWiFiModem


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
<h2>Software LED is <span id="state">{state}</span></h2>
<button id="toggle" onclick="toggleLed()" style="width:220px;height:80px;font-size:28px;">
Toggle
</button>
<script>
function toggleLed(){{
  const b = document.getElementById('toggle');
  b.disabled = true;
  fetch('/toggle')
    .then(r => r.text())
    .then(t => {{ document.getElementById('state').textContent = t; }})
    .finally(() => {{ b.disabled = false; }});
}}
</script>
</body>
</html>"""


def request_path(request_bytes):
    first_line = request_bytes.decode("iso-8859-1", errors="replace").splitlines()[0]
    parts = first_line.split()
    if len(parts) >= 2:
        return parts[1]
    return "/"


def main():
    parser = argparse.ArgumentParser(description="MOD-ESP32-C5 Linux web demo")
    parser.add_argument("--port", default="/dev/ttyS1", help="UEXT UART device")
    parser.add_argument("--ssid", required=True, help="WiFi network name")
    parser.add_argument("--password", required=True, help="WiFi password")
    parser.add_argument("--http-port", type=int, default=80, help="HTTP port")
    args = parser.parse_args()

    modem = OlimexWiFiModem(port=args.port, debug=True)
    led_state = False

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
                request = modem.server_read(client_id)
                path = request_path(request)
                print(f"Request path: {path}")

                if path == "/favicon.ico":
                    modem.server_write(client_id, no_content_response())
                elif path == "/toggle":
                    led_state = not led_state
                    state = "ON" if led_state else "OFF"
                    print(f"Software LED state changed to: {state}")
                    modem.server_write(client_id, http_response(state, "text/plain"))
                else:
                    modem.server_write(client_id, http_response(page(led_state)))

            finally:
                modem.server_close(client_id)
                print("Client closed")

    finally:
        modem.close()


if __name__ == "__main__":
    main()


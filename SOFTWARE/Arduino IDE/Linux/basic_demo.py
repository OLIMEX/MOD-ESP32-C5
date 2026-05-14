#!/usr/bin/env python3

import argparse

from olimex_wifi_modem import OlimexWiFiModem


def main():
    parser = argparse.ArgumentParser(description="MOD-ESP32-C5 basic WiFi test")
    parser.add_argument("--port", default="/dev/ttyS1", help="UEXT UART device")
    parser.add_argument("--ssid", required=True, help="WiFi network name")
    parser.add_argument("--password", required=True, help="WiFi password")
    args = parser.parse_args()

    modem = OlimexWiFiModem(port=args.port, debug=True)

    try:
        print("================================")
        print("MOD-ESP32-C5 Linux Basic Demo")
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

    finally:
        modem.close()


if __name__ == "__main__":
    main()


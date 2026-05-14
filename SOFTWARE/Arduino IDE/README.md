# MOD-ESP32-C5 Arduino IDE Software

This directory contains the Arduino IDE library, MOD-ESP32-C5 firmware example,
host board demos, and Linux userspace demos for using MOD-ESP32-C5 as a WiFi
modem over UEXT/UART.

## Directory Layout

```text
Arduino IDE/
  README.md
  OlimexWiFiModem.zip
  OlimexWiFiModem/
    library.properties
    README.md
    src/
    examples/
      ESP32_C5_Firmware/
      BasicDemo/
      WebServerDemo/
  Linux/
    README.md
    requirements.txt
    olimex_wifi_modem.py
    basic_demo.py
    web_server_demo.py
```

## Arduino IDE Library

Use `OlimexWiFiModem.zip` for the normal Arduino IDE install flow:

1. Open Arduino IDE.
2. Select `Sketch -> Include Library -> Add .ZIP Library...`.
3. Select `OlimexWiFiModem.zip`.
4. Open examples from `File -> Examples -> OlimexWiFiModem`.

The unzipped `OlimexWiFiModem/` folder is included for source browsing,
development, and GitHub visibility. Its internal structure is the standard
Arduino library layout.

Important examples:

- `ESP32_C5_Firmware` - upload this to MOD-ESP32-C5.
- `BasicDemo` - upload this to an Arduino-compatible host board to test WiFi.
- `WebServerDemo` - upload this to a host board to serve a browser LED button.

Read [OlimexWiFiModem/README.md](OlimexWiFiModem/README.md) for the full
Arduino setup guide.

## Linux Examples

The `Linux/` folder contains Python examples for OLinuXino boards and other
Linux hosts connected to MOD-ESP32-C5 over UEXT/UART.

The MOD-ESP32-C5 must already be flashed with the Arduino
`ESP32_C5_Firmware` example before using the Linux scripts.

Read [Linux/README.md](Linux/README.md) for Linux installation and usage.

## Tested Notes

- OLIMEXINO-2560 host examples use `Serial1` for UEXT.
- A20-OLinuXino-MICRO UEXT1 was tested as `/dev/ttyS4`.
- A20-OLinuXino-MICRO LED1 is controlled through `/sys/class/leds`.
- UART speed is `115200 baud`.

For other boards, adjust the host serial port or Linux `/dev/ttySx` device to
match the board schematic and image.

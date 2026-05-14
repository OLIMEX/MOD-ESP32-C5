# OlimexWiFiModem

Arduino IDE library and examples for using MOD-ESP32-C5 as a WiFi modem over
UEXT/UART.

The main target setup is:

- Main board: OLIMEXINO-2560
- WiFi modem board: MOD-ESP32-C5
- Link between boards: UEXT UART at 115200 baud

The firmware can also be used with other host boards that can talk over a UART,
for example other Arduino-compatible boards or OLIMEX Linux boards connected to
UEXT. The host side must send the same text commands described below.

## What Is In This Folder

```text
OlimexWiFiModem/
  library.properties
  README.md
  src/
    OlimexWiFiModem.h
    OlimexWiFiModem.cpp
  examples/
    ESP32_C5_Firmware/
      ESP32_C5_Firmware.ino
    BasicDemo/
      BasicDemo.ino
    WebServerDemo/
      WebServerDemo.ino
```

`ESP32_C5_Firmware` is uploaded to MOD-ESP32-C5.

`BasicDemo` and `WebServerDemo` are uploaded to the host board, for example
OLIMEXINO-2560.

## Install In Arduino IDE From ZIP

This folder is meant to be zipped and installed directly in Arduino IDE.

1. Zip the whole `OlimexWiFiModem` folder.
2. Open Arduino IDE.
3. Select `Sketch -> Include Library -> Add .ZIP Library...`.
4. Select the ZIP file.
5. Open the examples from `File -> Examples -> OlimexWiFiModem`.

After installation, Arduino IDE should show these examples:

- `ESP32_C5_Firmware`
- `BasicDemo`
- `WebServerDemo`

## Hardware Setup

1. Connect MOD-ESP32-C5 to the host board through UEXT.
2. Power the boards as recommended for your hardware.
3. Use a USB cable for the board you are programming.
4. Open Serial Monitor at `115200 baud`.

For OLIMEXINO-2560, the examples use:

```cpp
HardwareSerial &ModemSerial = Serial1;
```

This means the library talks to MOD-ESP32-C5 through `Serial1`.

For another host board, change this line to the hardware serial port connected
to the UEXT UART.

## Step 1: Upload The MOD-ESP32-C5 Firmware

Open:

`File -> Examples -> OlimexWiFiModem -> ESP32_C5_Firmware`

Select the correct ESP32-C5 board and port in Arduino IDE, then upload.

After reset, open Serial Monitor at `115200 baud`. Expected output:

```text
READY
>
```

This means the firmware is running and waiting for commands from the host board.

## Step 2: Run BasicDemo On The Host Board

Open:

`File -> Examples -> OlimexWiFiModem -> BasicDemo`

Before uploading, edit these two lines:

```cpp
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"
```

Example:

```cpp
#define WIFI_SSID "MyRouter"
#define WIFI_PASSWORD "MyPassword"
```

Upload the sketch to OLIMEXINO-2560 or another host board.

Open Serial Monitor at `115200 baud`. Expected output will look similar to:

```text
================================
OlimexWiFiModem Basic Demo
================================
Checking MOD-ESP32-C5...
[TX] ping
[RX] PONG
Modem OK
Connecting to WiFi...
[TX] connect MyRouter MyPassword
[RX] WIFI:CONNECTED
WiFi connected
[TX] ip
[RX] IP:192.168.0.198
IP: 192.168.0.198
Basic demo finished.
```

Your IP address will probably be different.

## Step 3: Run WebServerDemo On The Host Board

Open:

`File -> Examples -> OlimexWiFiModem -> WebServerDemo`

Before uploading, edit these two lines:

```cpp
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"
```

Upload the sketch and open Serial Monitor at `115200 baud`.

Expected startup output:

```text
================================
OLIMEXINO-2560 Web Server Demo
================================
[TX] ping
[RX] PONG
Modem OK
Connecting to WiFi...
[TX] connect MyRouter MyPassword
[RX] WIFI:CONNECTED
WiFi connected
[TX] ip
[RX] IP:192.168.0.198
Module IP: 192.168.0.198
[TX] server start 80
[RX] SERVER:STARTED
HTTP server started
Open browser: http://192.168.0.198
```

Open the printed address in a browser connected to the same network.

Expected result:

- A web page opens with title `OLIMEXINO-2560`.
- The page shows `LED is OFF` or `LED is ON`.
- Pressing `Toggle LED` changes the board LED on pin 13.
- The page updates quickly without a full page reload.

When the browser connects, Serial Monitor will show request/response activity:

```text
[RX] SERVER:CLIENT:0
Client connected, ID=0
[TX] server read 0
[RX] DATA:304
Request: GET /toggle HTTP/1.1
LED state changed to: ON
[TX] server write 0 91
[RX] READY
[RX] OK
[TX] server close 0
[RX] OK
Client closed
```

The exact byte counts can be different.

## How The Firmware And Library Communicate

The host board sends text commands to MOD-ESP32-C5 over UART. Each command ends
with a newline.

Common commands:

```text
ping
connect <ssid> <password>
ip
server start <port>
server read <client_id>
server write <client_id> <byte_count>
server close <client_id>
```

Common firmware replies:

```text
PONG
WIFI:CONNECTED
IP:192.168.0.198
SERVER:STARTED
SERVER:CLIENT:0
DATA:304
READY
OK
ERROR:...
```

For HTTP server use, MOD-ESP32-C5 accepts the browser connection and notifies
the host board with `SERVER:CLIENT:<id>`. The host board then reads the browser
request, writes an HTTP response, and closes the browser client.

## Troubleshooting

If Serial Monitor shows `Modem not found`:

- Check that MOD-ESP32-C5 is flashed with `ESP32_C5_Firmware`.
- Check UEXT connection and board power.
- Check that the host sketch uses the correct serial port.
- Make sure both sides use `115200 baud`.

If Serial Monitor shows `WiFi failed`:

- Check the SSID and password.
- Make sure the WiFi network is in range.
- Use a 2.4 GHz network if your ESP32-C5 setup does not support the selected
  router mode.

If the web page does not open:

- Confirm that `HTTP server started` is printed.
- Confirm that your computer or phone is on the same network.
- Open the exact IP address printed by the Serial Monitor.
- Try refreshing the browser after the board prints a new IP address.

If the button is slow or stops responding:

- Reflash MOD-ESP32-C5 with the firmware from this library.
- Reupload the latest `WebServerDemo`.
- Watch Serial Monitor for `ERROR:` messages.

## Using Other Host Boards

The library is written for Arduino-style `Stream` objects. This means the host
board only needs a serial port connected to the MOD-ESP32-C5 UEXT UART.

For boards such as OLIMEXINO-328, the serial port may be different from
OLIMEXINO-2560. Update this line in the demos:

```cpp
HardwareSerial &ModemSerial = Serial1;
```

Use the serial object that matches your wiring.

For Linux boards, the same protocol can be used from a userspace program that
opens the UEXT serial device at `115200 baud` and sends the text commands shown
above.

## Notes For Beginners

- Upload `ESP32_C5_Firmware` to MOD-ESP32-C5 first.
- Upload `BasicDemo` or `WebServerDemo` to the main board second.
- Always edit `WIFI_SSID` and `WIFI_PASSWORD` before uploading host demos.
- Keep Serial Monitor open at `115200 baud`; it is the easiest way to see what
  is happening.
- The IP address is assigned by your router, so it may change between tests.

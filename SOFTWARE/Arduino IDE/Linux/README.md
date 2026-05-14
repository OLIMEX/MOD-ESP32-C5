# Olinuxino Example For MOD-ESP32-C5

This folder contains Linux userspace examples for using MOD-ESP32-C5 from an
OLinuXino board such as A20-OLinuXino-MICRO.

The MOD-ESP32-C5 firmware is assumed to be already uploaded and working. These
files do not modify the ESP32-C5 firmware.

## Files

```text
Olinuxino-example/
  README.md
  requirements.txt
  olimex_wifi_modem.py
  basic_demo.py
  web_server_demo.py
```

- `olimex_wifi_modem.py` - small Python driver for the MOD-ESP32-C5 UART protocol.
- `basic_demo.py` - checks the modem, connects to WiFi, and prints the IP address.
- `web_server_demo.py` - starts the MOD-ESP32-C5 web server and shows a browser button.
- `requirements.txt` - Python dependency list.

## Copy Files To The OLinuXino Board

Copy the whole `Olinuxino-example` folder to the Linux board.

Example from your PC:

```sh
scp -r Olinuxino-example root@OLINUXINO_IP:/root/
```

Or copy it with a USB flash drive, SD card, `sftp`, or any other method.

Then log in to the OLinuXino board:

```sh
ssh root@OLINUXINO_IP
cd /root/Olinuxino-example
```

## Install Python Dependency

On Debian-based OLinuXino images:

```sh
apt update
apt install python3 python3-serial
```

Alternative using `pip`:

```sh
python3 -m pip install -r requirements.txt
```

## Connect Hardware

1. Flash MOD-ESP32-C5 with the Arduino `ESP32_C5_Firmware` first.
2. Connect MOD-ESP32-C5 to the OLinuXino board through UEXT.
3. Power the boards.
4. Find the Linux serial device for UEXT.

The examples default to:

```text
/dev/ttyS1
```

Your image may use another UART device. Check with:

```sh
ls -l /dev/ttyS*
dmesg | grep tty
```

If `/dev/ttyS1` does not work, try `/dev/ttyS2`, `/dev/ttyS3`, etc.

## Run The Basic Demo

Replace `WIFI_SSID` and `WIFI_PASSWORD` with your real WiFi credentials:

```sh
python3 basic_demo.py --port /dev/ttyS1 --ssid WIFI_SSID --password WIFI_PASSWORD
```

Expected output:

```text
================================
MOD-ESP32-C5 Linux Basic Demo
================================
Serial port: /dev/ttyS1
Checking modem...
[TX] ping
[RX] PONG
Modem OK
Connecting to WiFi...
[TX] connect MyRouter MyPassword
[RX] WIFI:CONNECTED
[TX] ip
[RX] IP:192.168.0.198
WiFi connected, IP: 192.168.0.198
```

Your IP address will be different.

## Run The Web Server Demo

Replace `WIFI_SSID` and `WIFI_PASSWORD` with your real WiFi credentials:

```sh
python3 web_server_demo.py --port /dev/ttyS1 --ssid WIFI_SSID --password WIFI_PASSWORD
```

Expected startup output:

```text
================================
MOD-ESP32-C5 Linux Web Demo
================================
Serial port: /dev/ttyS1
Checking modem...
[TX] ping
[RX] PONG
Modem OK
Connecting to WiFi...
[TX] connect MyRouter MyPassword
[RX] WIFI:CONNECTED
[TX] ip
[RX] IP:192.168.0.198
WiFi connected, IP: 192.168.0.198
[TX] server start 80
[RX] SERVER:STARTED
HTTP server started
Open browser: http://192.168.0.198
```

Open the printed address in a browser connected to the same network.

Expected result:

- A web page titled `OLinuXino + MOD-ESP32-C5` opens.
- The page shows `Software LED is OFF` or `Software LED is ON`.
- Pressing the button toggles the software state.
- Serial output shows each browser request.

This demo does not toggle a physical OLinuXino GPIO pin. It only proves that the
Linux board can control the MOD-ESP32-C5 firmware over UEXT and serve a browser
page through it.

## Troubleshooting

If you see `Modem not detected`:

- Check that MOD-ESP32-C5 is powered.
- Check that the MOD-ESP32-C5 firmware is already uploaded.
- Check the UEXT cable/connection.
- Try another serial port such as `/dev/ttyS2`.
- Make sure nothing else is using the same serial port.

If you see `WiFi failed`:

- Check the SSID and password.
- Make sure the router is in range.
- Make sure the WiFi network mode is supported by your MOD-ESP32-C5 setup.

If the web page does not open:

- Make sure your browser device is on the same network.
- Use the IP address printed by the demo.
- Check for firewall or routing issues on the network.

## Protocol Summary

The Linux scripts send simple text commands to the MOD-ESP32-C5 firmware:

```text
ping
connect <ssid> <password>
ip
server start <port>
server read <client_id>
server write <client_id> <byte_count>
server close <client_id>
```

The firmware replies with messages such as:

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


# OLinuXino Example For MOD-ESP32-C5

Linux userspace examples for using MOD-ESP32-C5 from an OLinuXino board.

The MOD-ESP32-C5 firmware is assumed to be already flashed with the Arduino
`ESP32_C5_Firmware` example. These files only run on the Linux host board.

## Files

```text
Olinuxino-example/
  README.md
  requirements.txt
  olimex_wifi_modem.py
  basic_demo.py
  web_server_demo.py
```

- `olimex_wifi_modem.py` - Python driver for the MOD-ESP32-C5 UART protocol.
- `basic_demo.py` - checks the modem, connects WiFi, and prints the IP address.
- `web_server_demo.py` - serves a browser button through MOD-ESP32-C5.

## Install Dependency

On Debian-based OLinuXino images:

```sh
apt update
apt install python3 python3-serial
```

## Download Directly On The Board

```sh
mkdir -p Olinuxino-example
cd Olinuxino-example

wget -O basic_demo.py https://github.com/OLIMEX/MOD-ESP32-C5/raw/refs/heads/main/SOFTWARE/Arduino%20IDE/Linux/basic_demo.py
wget -O olimex_wifi_modem.py https://github.com/OLIMEX/MOD-ESP32-C5/raw/refs/heads/main/SOFTWARE/Arduino%20IDE/Linux/olimex_wifi_modem.py
wget -O web_server_demo.py https://github.com/OLIMEX/MOD-ESP32-C5/raw/refs/heads/main/SOFTWARE/Arduino%20IDE/Linux/web_server_demo.py
```

## Serial Port

For A20-OLinuXino-MICRO with MOD-ESP32-C5 connected to UEXT1, the schematic
shows A20 UART6 on UEXT1. On the tested Linux image this appears as:

```text
/dev/ttyS4
```

The examples default to `/dev/ttyS4`.

For another OLinuXino board, another UEXT connector, or another Linux image,
change `--port /dev/ttyS4` to the correct `/dev/ttySx`.

Useful checks:

```sh
ls -l /dev/ttyS*
dmesg | grep tty
```

## Basic Demo

```sh
python3 basic_demo.py --port /dev/ttyS4 --ssid WIFI_SSID --password WIFI_PASSWORD
```

Expected output:

```text
================================
MOD-ESP32-C5 Linux Basic Demo
================================
Serial port: /dev/ttyS4
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

## Web Server Demo

```sh
python3 web_server_demo.py --port /dev/ttyS4 --ssid WIFI_SSID --password WIFI_PASSWORD
```

Open the printed IP address in a browser on the same network.

Expected result:

- Page title: `OLinuXino + MOD-ESP32-C5`
- Button toggles LED1 on A20-OLinuXino-MICRO.
- Browser refreshes and stale connections are handled without stopping the script.

On A20-OLinuXino-MICRO, LED1 is exposed by the kernel LED subsystem. The web
demo selects a likely LED automatically. To choose one explicitly:

```sh
ls -l /sys/class/leds
python3 web_server_demo.py --port /dev/ttyS4 --ssid WIFI_SSID --password WIFI_PASSWORD --led LED_NAME
```

To run the web demo without controlling a physical LED:

```sh
python3 web_server_demo.py --port /dev/ttyS4 --ssid WIFI_SSID --password WIFI_PASSWORD --no-led
```

## Notes

- Run as `root` if your Linux image restricts serial or LED access.
- If the log shows `DATA:0`, `ERROR:BAD_ID`, or `ERROR:CLOSED` during browser
  refreshes, this usually means the browser opened or closed an extra socket.
  The latest scripts treat this as normal and keep running.
- If the log shows split protocol text such as `S` followed by
  `ERVER:CLIENT:3`, update `olimex_wifi_modem.py`; the driver reassembles known
  fragmented protocol lines.

## Protocol Summary

The Linux scripts send newline-terminated text commands over UEXT UART:

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

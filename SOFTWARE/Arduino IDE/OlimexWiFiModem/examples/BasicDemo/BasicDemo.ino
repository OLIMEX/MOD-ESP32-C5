#include <OlimexWiFiModem.h>

// Replace these two strings with the name and password of your WiFi network.
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"

// OLIMEXINO-2560 has the UEXT serial connection on Serial1.
// If you use another board, change this to the serial port wired to MOD-ESP32-C5.
HardwareSerial &ModemSerial = Serial1;

// The first parameter is the serial port connected to MOD-ESP32-C5.
// The second parameter is optional debug output printed to the Serial Monitor.
OlimexWiFiModem wifi(
  ModemSerial,
  &Serial
);

void setup() {

  // USB serial for the Arduino IDE Serial Monitor.
  Serial.begin(115200);

  // UEXT UART between the main board and MOD-ESP32-C5.
  ModemSerial.begin(115200);

  Serial.println();
  Serial.println("================================");
  Serial.println("OlimexWiFiModem Basic Demo");
  Serial.println("================================");

  Serial.println("Checking MOD-ESP32-C5...");

  if (!wifi.begin()) {

    Serial.println("Modem not found");

    while (1);
  }

  Serial.println("Modem OK");
  Serial.println("Connecting to WiFi...");

  if (!wifi.connect(
        WIFI_SSID,
        WIFI_PASSWORD
      )) {

    Serial.println("WiFi failed");

    while (1);
  }

  Serial.println("WiFi connected");

  Serial.print("IP: ");
  Serial.println(wifi.ip());

  Serial.println("Basic demo finished.");
}

void loop() {
  // Nothing is needed here. This demo only checks the modem and WiFi link.
}

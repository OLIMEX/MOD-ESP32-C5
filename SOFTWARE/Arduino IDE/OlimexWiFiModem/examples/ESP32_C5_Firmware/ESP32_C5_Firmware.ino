#include <WiFi.h>
#include <Preferences.h>

// UEXT UART pins used by MOD-ESP32-C5.
// These are the pins that connect the ESP32-C5 module to the host board.
#define UART_RX 4
#define UART_TX 5

// Status LEDs on MOD-ESP32-C5. They are active-low on this hardware.
#define LED_GREEN 27
#define LED_RED   26

// UART port used for the UEXT connector.
HardwareSerial UEXT(1);

// Preferences stores the last successful WiFi credentials in ESP32 flash.
Preferences prefs;

// The ESP32-C5 accepts browser connections and forwards them to the host board
// using simple text commands over UEXT.
WiFiServer server(80);

WiFiClient clients[8];
bool clientUsed[8];

// One command line received from the host board.
String rxLine;

// =====================================================

void ledGreen(bool on) {
  digitalWrite(LED_GREEN, on ? LOW : HIGH);
}

void ledRed(bool on) {
  digitalWrite(LED_RED, on ? LOW : HIGH);
}

void out(const String &s) {
  // Send every protocol message to both the host board and the USB Serial
  // Monitor. This makes debugging much easier for first-time users.
  UEXT.println(s);
  Serial.println(s);
}

void prompt() {
  // Human-friendly prompt. The host-side library ignores it.
  UEXT.print("> ");
}

void clearClients() {

  for (int i = 0; i < 8; i++) {

    if (clientUsed[i]) {
      clients[i].stop();
    }

    clientUsed[i] = false;
  }
}

// =====================================================

void connectWiFi(const String &ssid, const String &pass) {

  // This firmware behaves as a WiFi station: it joins an existing router.
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t start = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < 15000) {

    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {

    // Green LED means WiFi is connected.
    ledGreen(true);
    ledRed(false);

    // Store credentials only after a successful connection.
    prefs.begin("wifi", false);

    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);

    prefs.end();

    out("WIFI:CONNECTED");
    out("IP:" + WiFi.localIP().toString());

  } else {

    // Red LED means the connection attempt failed.
    ledGreen(false);
    ledRed(true);

    out("ERROR:CONNECT");
  }
}

// =====================================================

void serverTask() {

  // Clean up browser connections that closed by themselves.

  for (int i = 0; i < 8; i++) {

    if (clientUsed[i]) {

      if (!clients[i].connected()) {

        clients[i].stop();
        clientUsed[i] = false;
      }
    }
  }

  WiFiClient c = server.available();

  if (!c)
    return;

  // Lower latency for small HTTP responses such as the LED toggle response.
  c.setNoDelay(true);
  c.setTimeout(50);

  for (int i = 0; i < 8; i++) {

    if (!clientUsed[i]) {

      clients[i] = c;

      clientUsed[i] = true;

      // Tell the host board that a browser client is waiting.
      out("SERVER:CLIENT:" + String(i));

      return;
    }
  }

  c.stop();
}

// =====================================================

void cmdServerRead(int id) {

  if (id < 0 || id >= 8 || !clientUsed[id]) {

    out("ERROR:BAD_ID");
    return;
  }

  WiFiClient &c = clients[id];

  if (!c.connected()) {

    c.stop();

    clientUsed[id] = false;

    out("ERROR:CLOSED");
    return;
  }

  String req;

  uint32_t start = millis();

  // Read one HTTP request header from the browser.
  // For these demos we only need the header, not a POST body.
  while (millis() - start < 2000) {

    while (c.available()) {

      char ch = c.read();

      req += ch;

      start = millis();

      if (req.length() > 1200) {

        // Protect RAM if a browser or client sends an unexpectedly large
        // request header.
        out("ERROR:TOO_LARGE");
        return;
      }

      if (req.endsWith("\r\n\r\n")) {

        // Framed response to the host:
        // DATA:<number of bytes>
        // <raw HTTP request bytes>
        // END
        out("DATA:" + String(req.length()));

        UEXT.print(req);

        out("");
        out("END");

        return;
      }
    }
  }

  out("DATA:" + String(req.length()));

  UEXT.print(req);

  out("");
  out("END");
}

// =====================================================

void cmdServerWrite(int id, int len) {

  if (id < 0 || id >= 8 || !clientUsed[id]) {

    out("ERROR:BAD_ID");
    return;
  }

  WiFiClient &c = clients[id];

  if (!c.connected()) {

    c.stop();

    clientUsed[id] = false;

    out("ERROR:CLOSED");
    return;
  }

  // Tell the host board it can now send exactly len bytes.
  out("READY");

  uint8_t *buf = (uint8_t *)malloc(len);

  if (!buf) {

    out("ERROR:MEM");
    return;
  }

  int received = 0;

  uint32_t start = millis();

  while (received < len &&
         millis() - start < 5000) {

    while (UEXT.available() &&
           received < len) {

      buf[received++] = UEXT.read();

      start = millis();
    }
  }

  if (received != len) {

    free(buf);

    out("ERROR:TIMEOUT");
    return;
  }

  size_t written = c.write(buf, len);

  // Push the bytes out to the browser before reporting OK to the host.
  c.flush();

  free(buf);

  if (written != (size_t)len) {

    c.stop();

    clientUsed[id] = false;

    out("ERROR:WRITE");
    return;
  }

  out("OK");
}

// =====================================================

void cmdServerClose(int id) {

  if (id < 0 || id >= 8) {

    out("ERROR:BAD_ID");
    return;
  }

  if (clientUsed[id]) {

    // Make sure any remaining outgoing bytes are pushed before closing.
    clients[id].flush();

    delay(20);

    clients[id].stop();

    clientUsed[id] = false;
  }

  out("OK");
}

// =====================================================

void processCommand(String cmd) {

  cmd.trim();

  // Commands accepted from the host board:
  //   ping
  //   connect <ssid> <password>
  //   ip
  //   server start <port>
  //   server read <client_id>
  //   server write <client_id> <byte_count>
  //   server close <client_id>
  if (cmd == "ping") {

    out("PONG");
  }

  else if (cmd.startsWith("connect ")) {

    int p = cmd.indexOf(' ', 8);

    if (p < 0) {

      out("ERROR:ARGS");
      return;
    }

    String ssid = cmd.substring(8, p);

    String pass = cmd.substring(p + 1);

    connectWiFi(ssid, pass);
  }

  else if (cmd == "ip") {

    out("IP:" + WiFi.localIP().toString());
  }

  else if (cmd.startsWith("server start ")) {

    int port = cmd.substring(13).toInt();

    server.stop();

    delay(100);

    clearClients();

    server = WiFiServer(port);

    server.begin();

    out("SERVER:STARTED");
  }

  else if (cmd.startsWith("server read ")) {

    int id = cmd.substring(12).toInt();

    cmdServerRead(id);
  }

  else if (cmd.startsWith("server write ")) {

    int p = cmd.indexOf(' ', 13);

    if (p < 0) {

      out("ERROR:ARGS");
      return;
    }

    int id = cmd.substring(13, p).toInt();

    int len = cmd.substring(p + 1).toInt();

    cmdServerWrite(id, len);
  }

  else if (cmd.startsWith("server close ")) {

    int id = cmd.substring(13).toInt();

    cmdServerClose(id);
  }

  else {

    out("ERROR:UNKNOWN");
  }

  prompt();
}

// =====================================================

void setup() {

  // USB serial for the Arduino IDE Serial Monitor while flashing/debugging
  // the MOD-ESP32-C5 firmware.
  Serial.begin(115200);

  // UEXT serial used by the main board library.
  UEXT.begin(
    115200,
    SERIAL_8N1,
    UART_RX,
    UART_TX
  );

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  ledGreen(false);
  ledRed(false);

  clearClients();

  // READY lets a human know that the firmware has booted.
  out("READY");

  prompt();
}

// =====================================================

void loop() {

  // Read command lines from the host board.
  while (UEXT.available()) {

    char c = UEXT.read();

    if (c == '\r')
      continue;

    if (c == '\n') {

      processCommand(rxLine);

      rxLine = "";

    } else {

      rxLine += c;
    }
  }

  // Accept new browser clients when the web server is running.
  serverTask();
}

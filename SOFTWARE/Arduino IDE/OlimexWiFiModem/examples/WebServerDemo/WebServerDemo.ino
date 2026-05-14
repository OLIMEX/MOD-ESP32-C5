#include <OlimexWiFiModem.h>

// Replace these two strings with the name and password of your WiFi network.
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"

// Built-in LED on many Arduino-compatible boards.
#define LED_PIN 13

// OLIMEXINO-2560 has the UEXT serial connection on Serial1.
// If you use another board, change this to the serial port wired to MOD-ESP32-C5.
HardwareSerial &ModemSerial = Serial1;

// The library talks to the MOD-ESP32-C5 over ModemSerial.
// Debug messages are printed to the USB Serial Monitor.
OlimexWiFiModem wifi(
  ModemSerial,
  &Serial
);

bool ledState = false;

// =====================================================

void sendHttp(
  int id,
  const String &contentType,
  const String &body,
  const String &status = "200 OK"
) {

  // Browsers behave best when the response length is known and the
  // connection is closed after each small embedded request.
  String resp = "HTTP/1.1 ";
  resp += status;
  resp += "\r\nContent-Type: ";
  resp += contentType;
  resp += "\r\nContent-Length: ";
  resp += String(body.length());
  resp += "\r\nConnection: close\r\n";
  resp += "Cache-Control: no-store\r\n\r\n";
  resp += body;

  wifi.serverWrite(
    id,
    (const uint8_t *)resp.c_str(),
    resp.length()
  );
}

// =====================================================

void sendLedState(int id) {

  // AJAX response for /toggle. The web page uses this short text to update
  // the LED state without reloading the full page.
  sendHttp(
    id,
    "text/plain",
    ledState ? "ON" : "OFF"
  );
}

// =====================================================

void sendNoContent(int id) {

  // Chrome and other browsers often ask for /favicon.ico automatically.
  // This small response says "there is no icon" and avoids extra errors.
  const char *resp =
    "HTTP/1.1 204 No Content\r\n"
    "Connection: close\r\n"
    "Cache-Control: no-store\r\n"
    "\r\n";

  wifi.serverWrite(
    id,
    (const uint8_t *)resp,
    strlen(resp)
  );
}

// =====================================================

void sendPage(int id) {

  // Simple one-page control panel. The JavaScript sends GET /toggle when the
  // button is pressed, then displays the returned ON/OFF state.
  String html =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>OLIMEXINO-2560</title>"
    "</head>"
    "<body style='font-family:Arial;text-align:center;margin-top:40px;'>"
    "<h1>OLIMEXINO-2560</h1>"
    "<h2>LED is <span id='state'>";

  html += ledState ? "ON" : "OFF";

  html += "</span></h2>"
          "<button id='toggle' onclick='toggleLed()' "
          "style='width:220px;height:80px;font-size:28px;'>"
          "Toggle LED"
          "</button>"

          "<script>"
          "function toggleLed(){"
          "const b=document.getElementById('toggle');"
          "const s=document.getElementById('state');"
          "const c=new AbortController();"
          "const t=setTimeout(()=>c.abort(),2500);"
          "b.disabled=true;"
          "fetch(\"/toggle\",{cache:\"no-store\",signal:c.signal})"
          ".then(r=>r.text())"
          ".then(x=>{s.textContent=x;})"
          ".catch(()=>{s.textContent=\"RETRY\";})"
          ".finally(()=>{clearTimeout(t);b.disabled=false;});"
          "}"
          "</script>"

          "</body>"
          "</html>";

  sendHttp(
    id,
    "text/html",
    html
  );
}

// =====================================================

String firstRequestLine(const String &req) {

  int end = req.indexOf('\n');

  if (end < 0) {
    return req;
  }

  String line = req.substring(0, end);

  line.trim();

  return line;
}

// =====================================================

void setup() {

  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);

  // USB serial for the Arduino IDE Serial Monitor.
  Serial.begin(115200);

  // UEXT UART between the main board and MOD-ESP32-C5.
  ModemSerial.begin(115200);

  Serial.println();
  Serial.println("================================");
  Serial.println("OLIMEXINO-2560 Web Server Demo");
  Serial.println("================================");

  if (!wifi.begin()) {

    Serial.println("Modem not detected");

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

  String ip = wifi.ip();

  Serial.print("Module IP: ");
  Serial.println(ip);

  if (!wifi.serverStart(80)) {

    Serial.println("Server failed");

    while (1);
  }

  Serial.println("HTTP server started");

  Serial.print("Open browser: http://");
  Serial.println(ip);
}

// =====================================================

void loop() {

  // serverAvailable() returns a client ID when the ESP32-C5 has accepted a
  // browser connection. It returns -1 when no browser request is waiting.
  int id = wifi.serverAvailable();

  if (id < 0)
    return;

  Serial.print("Client connected, ID=");
  Serial.println(id);

  uint8_t reqBuf[768];

  // Read the HTTP request from the browser through the ESP32-C5.
  int len = wifi.serverRead(
    id,
    reqBuf,
    sizeof(reqBuf) - 1,
    2000
  );

  if (len <= 0) {

    Serial.println("ERROR: no request data");

    wifi.serverClose(id);

    return;
  }

  reqBuf[len] = 0;

  String req = (char *)reqBuf;

  Serial.print("Request: ");
  Serial.println(firstRequestLine(req));

  // favicon

  if (req.indexOf("GET /favicon.ico") >= 0) {

    sendNoContent(id);

    wifi.serverClose(id);

    return;
  }

  // toggle LED

  if (req.indexOf("GET /toggle") >= 0) {

    ledState = !ledState;

    digitalWrite(
      LED_PIN,
      ledState ? HIGH : LOW
    );

    Serial.print("LED state changed to: ");

    Serial.println(
      ledState ? "ON" : "OFF"
    );

    sendLedState(id);

    wifi.serverClose(id);

    Serial.println("Client closed");

    return;
  }

  // Any other request, including GET /, receives the main web page.
  sendPage(id);

  wifi.serverClose(id);

  Serial.println("Client closed");
}

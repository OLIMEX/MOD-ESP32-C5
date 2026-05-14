#include "OlimexWiFiModem.h"

OlimexWiFiModem::OlimexWiFiModem(
  Stream &serial,
  Stream *debug
)
: _serial(serial),
  _debug(debug),
  _pendingClientCount(0) {
}

bool OlimexWiFiModem::begin(uint32_t timeout) {

  flushInput();

  sendCmd("ping");

  return waitFor("PONG", timeout);
}

void OlimexWiFiModem::flushInput() {

  while (_serial.available()) {
    _serial.read();
  }
}

void OlimexWiFiModem::sendCmd(const String &cmd) {

  if (_debug) {
    _debug->print("[TX] ");
    _debug->println(cmd);
  }

  _serial.print(cmd);
  _serial.print('\n');
}

void OlimexWiFiModem::cacheClient(int id) {

  if (id < 0) {
    return;
  }

  if (_pendingClientCount >= 8) {
    return;
  }

  _pendingClients[_pendingClientCount++] = id;
}

bool OlimexWiFiModem::cacheClientLine(const String &line) {

  if (!line.startsWith("SERVER:CLIENT:")) {
    return false;
  }

  cacheClient(line.substring(14).toInt());

  return true;
}

bool OlimexWiFiModem::waitFor(
  const String &token,
  uint32_t timeout
) {

  uint32_t start = millis();

  while (millis() - start < timeout) {

    String line = readLine(100);

    if (line.length()) {

      if (_debug) {
        _debug->print("[RX] ");
        _debug->println(line);
      }

      cacheClientLine(line);

      if (line.indexOf(token) >= 0) {
        return true;
      }
    }
  }

  return false;
}

String OlimexWiFiModem::readLine(uint32_t timeout) {

  String s;

  uint32_t start = millis();

  while (millis() - start < timeout) {

    while (_serial.available()) {

      char c = _serial.read();

      if (c == '\r')
        continue;

      if (c == '\n') {

        s.trim();

        if (s.length()) {

          return s;
        }

        s = "";

      } else {

        // ignore prompt chars
        if (c == '>') {

          if (s.length() == 0)
            continue;
        }

        s += c;
      }

      start = millis();
    }
  }

  s.trim();

  return s;
}

size_t OlimexWiFiModem::readBytes(
  uint8_t *buffer,
  size_t len,
  uint32_t timeout
) {

  size_t pos = 0;

  uint32_t start = millis();

  while (pos < len && millis() - start < timeout) {

    while (_serial.available() && pos < len) {

      buffer[pos++] = _serial.read();

      start = millis();
    }
  }

  return pos;
}

bool OlimexWiFiModem::connect(
  const String &ssid,
  const String &pass
) {

  flushInput();

  sendCmd("connect " + ssid + " " + pass);

  return waitFor("WIFI:CONNECTED", 15000);
}

bool OlimexWiFiModem::disconnect() {

  flushInput();

  sendCmd("disconnect");

  return waitFor("OK", 3000);
}

bool OlimexWiFiModem::isConnected() {

  flushInput();

  sendCmd("status");

  return waitFor("WIFI:CONNECTED", 3000);
}

String OlimexWiFiModem::ip() {

  flushInput();

  sendCmd("ip");

  uint32_t start = millis();

  while (millis() - start < 3000) {

    String line = readLine(3000);

    line.trim();

    if (_debug && line.length()) {
      _debug->print("[RX] ");
      _debug->println(line);
    }

    // remove prompt contamination
    if (line.startsWith(">")) {

      line.remove(0, 1);

      line.trim();
    }

    if (line.startsWith("IP:")) {

      return line.substring(3);
    }
  }

  return "";
}

bool OlimexWiFiModem::serverStart(uint16_t port) {

  flushInput();

  sendCmd("server start " + String(port));

  return waitFor("SERVER:STARTED", 3000);
}

int OlimexWiFiModem::serverAvailable() {

  if (_pendingClientCount > 0) {

    int id = _pendingClients[0];

    for (uint8_t i = 1; i < _pendingClientCount; i++) {
      _pendingClients[i - 1] = _pendingClients[i];
    }

    _pendingClientCount--;

    return id;
  }

  String line = readLine(10);

  if (line.startsWith("SERVER:CLIENT:")) {

    if (_debug) {
      _debug->print("[RX] ");
      _debug->println(line);
    }

    return line.substring(14).toInt();
  }

  return -1;
}

int OlimexWiFiModem::serverRead(
  int id,
  uint8_t *buffer,
  size_t maxLen,
  uint32_t timeout
) {

  sendCmd("server read " + String(id));

  uint32_t start = millis();

  while (millis() - start < timeout) {

    String line = readLine(100);

    if (!line.length()) {
      continue;
    }

    if (_debug) {
      _debug->print("[RX] ");
      _debug->println(line);
    }

    if (line.startsWith("ERROR:")) {
      return -1;
    }

    if (cacheClientLine(line)) {
      continue;
    }

    if (!line.startsWith("DATA:")) {
      continue;
    }

    size_t dataLen = line.substring(5).toInt();
    size_t copyLen = dataLen;

    if (copyLen > maxLen) {
      copyLen = maxLen;
    }

    size_t copied = readBytes(buffer, copyLen, timeout);

    if (copied != copyLen) {
      return -1;
    }

    uint8_t discard;

    for (size_t i = copyLen; i < dataLen; i++) {

      if (readBytes(&discard, 1, timeout) != 1) {
        return -1;
      }
    }

    waitFor("END", 500);

    return copyLen;
  }

  return -1;
}

bool OlimexWiFiModem::serverWrite(
  int id,
  const uint8_t *data,
  size_t len
) {

  sendCmd(
    "server write " +
    String(id) +
    " " +
    String(len)
  );

  if (!waitFor("READY", 3000)) {
    return false;
  }

  _serial.write(data, len);

  return waitFor("OK", 3000);
}

bool OlimexWiFiModem::serverWrite(
  int id,
  const String &s
) {

  return serverWrite(
    id,
    (const uint8_t*)s.c_str(),
    s.length()
  );
}

bool OlimexWiFiModem::serverClose(int id) {

  sendCmd("server close " + String(id));

  return waitFor("OK", 3000);
}

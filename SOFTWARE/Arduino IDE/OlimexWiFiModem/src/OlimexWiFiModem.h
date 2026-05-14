#pragma once

#include <Arduino.h>

class OlimexWiFiModem {
public:
  explicit OlimexWiFiModem(
    Stream &serial,
    Stream *debug = nullptr
  );

  bool begin(uint32_t timeout = 3000);

  bool connect(
    const String &ssid,
    const String &pass
  );

  bool disconnect();

  bool isConnected();

  String ip();

  bool serverStart(uint16_t port);

  int serverAvailable();

  int serverRead(
    int id,
    uint8_t *buffer,
    size_t maxLen,
    uint32_t timeout = 2000
  );

  bool serverWrite(
    int id,
    const uint8_t *data,
    size_t len
  );

  bool serverWrite(
    int id,
    const String &s
  );

  bool serverClose(int id);

private:
  Stream &_serial;
  Stream *_debug;
  int _pendingClients[8];
  uint8_t _pendingClientCount;

  void flushInput();

  void sendCmd(const String &cmd);

  void cacheClient(int id);

  bool cacheClientLine(const String &line);

  bool waitFor(
    const String &token,
    uint32_t timeout
  );

  size_t readBytes(
    uint8_t *buffer,
    size_t len,
    uint32_t timeout
  );

  String readLine(uint32_t timeout = 2000);
};

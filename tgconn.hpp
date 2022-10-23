#ifndef TGCONN_H
#define TGCONN_H

#include <Arduino.h>

class TgConn {
public:
  using inputMessageCB_t = void(*)(const char *rpID, const char *rpMessage);
  TgConn();
  void setup(inputMessageCB_t rfEventCB);
  void process();
  void reportStatus();
  void disconnect();
  void sendMessage(const char *rpID, const char *rpMessage);
  unsigned getDayTime();
private:
  void printWiFiStatus(int rdStatusIdx);
  inputMessageCB_t mfInputMessageCB = nullptr;
  unsigned long mdPowerUpTimeSync = 0;
  unsigned long mdEpochTimeSync = 0;
};

extern TgConn gTgConn;

#endif // TGCONN_H
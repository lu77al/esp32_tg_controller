#ifndef TGCONN_H
#define TGCONN_H

#include <Arduino.h>

class TgConn {
public:
  using eventCB_t = void(*)(const char *pEvent);
  TgConn();
  void setup(eventCB_t rfEventCB);
  void process();
  void reportStatus();
  // void connect_wifi();
  // void connect_wifi(const char *raSSID, const char *raPassword);
  void disconnect();
  // void check_messages();
private:
  void printWiFiStatus(int rdStatusIdx);

//   bool mbWiFiConnected;
//   unsigned long mdNextTime;
};

extern TgConn gTgConn;

#endif // TGCONN_H
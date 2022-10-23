#include <string.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include "shell.hpp"
#include "tgconn.hpp"
#if __has_include("credentials.hpp")
#include "credentials.hpp"
#else
#define CHAT_ID "_CHAT_ID_"
#endif // __has_include("credentials.hpp")

constexpr char gaCHAT_ID[] = CHAT_ID;
constexpr uint8_t gdLED_PIN = 23;

enum class LedState {
  On,
  Off,
  Toggle
};

OneWire gtDallasBus(22);
DallasTemperature gtDallas(&gtDallasBus);

LedState geLedState = LedState::Off;
unsigned long gdDallasProcessTime;
float gdTemp = 0.0;

void processShellCmd();
void processTgInputMessage(const char *rpID, const char *rpInputMessage);

void turnLed(LedState rdState) {
  geLedState = LedState::Toggle == rdState ? (LedState::On == geLedState ? LedState::Off : LedState::On) : rdState;
  digitalWrite(gdLED_PIN, LedState::On == geLedState ? LOW : HIGH);
}

void setup() {
  gShell.setup(processShellCmd);
  gTgConn.setup(processTgInputMessage);
  pinMode(gdLED_PIN, OUTPUT);
  turnLed(LedState::Off);
  gtDallas.begin();
  gdDallasProcessTime = millis() + 1000;
}

void loop() {
  gShell.process();
  gTgConn.process();
  if (millis() > gdDallasProcessTime) {
    turnLed(LedState::Toggle);
    gtDallas.requestTemperatures();
    gdTemp = gtDallas.getTempCByIndex(0);
    gdDallasProcessTime = millis() + 5500;
    gShell.clear();
    Serial.printf("t=%+.1fºC"NL, gdTemp);
    turnLed(LedState::Toggle);
  }
}

void processTgInputMessage(const char *rpID, const char *rpInputMessage) {
  if (0 == strcmp(rpID, gaCHAT_ID)) {
    if (0 == strcmp("/on", rpInputMessage)) {
      turnLed(LedState::On);
      gTgConn.sendMessage(gaCHAT_ID, "Turn LED on");
    } else if (0 == strcmp("/off", rpInputMessage)) {
      turnLed(LedState::Off);
      gTgConn.sendMessage(gaCHAT_ID, "Turn LED off");
    } else if (0 == strcmp("/tmp", rpInputMessage)) {
      char aReport[16];
      sprintf(aReport, "t=%+.1fºC"NL, gdTemp);
      gTgConn.sendMessage(gaCHAT_ID, aReport);
    } else if (0 == strncmp("/cmd", rpInputMessage, 4)) {
      const int dInputLen = strlen(rpInputMessage);
      if (dInputLen >= 6) {
        char *cmd = (char *)malloc(dInputLen - 4);
        memcpy(cmd, rpInputMessage + 5, dInputLen - 4);
        Serial.println(cmd);
        gShell.command(cmd);
        free(cmd);
      }
    } else {
      gTgConn.sendMessage(gaCHAT_ID, "Command unknown. Please use:\n\n"
        "/on  - turn LED on\n\n"
        "/off - turn LED off\n\n"
        "/tmp - get temperature"
      );
    }
  }
}

void processShellCmd(unsigned rdWordNum) {
  const char *rcmd = gShell.getWord(0);
  if (!strcmp("on", rcmd)) {
    turnLed(LedState::On);
  } else if (!strcmp("off", rcmd)) {
    turnLed(LedState::Off);
  } else if (!strcmp("dis", rcmd)) {
    gTgConn.disconnect();
  } else if (!strcmp("status", rcmd)) {
    gTgConn.reportStatus();
  } else if (!strcmp("time", rcmd)) {
    unsigned dDayTime = gTgConn.getDayTime();
    unsigned dSec = dDayTime % 60;
    dDayTime = (dDayTime - dSec) / 60;
    unsigned dMin = dDayTime % 60;
    unsigned dHour = (dDayTime - dMin) / 60;
    Serial.printf("%02d:%02d:%02d"NL, dHour, dMin, dSec);
  } else if (*rcmd) {
    Serial.print("Unknown command: ");
    Serial.println(rcmd);
  }
}

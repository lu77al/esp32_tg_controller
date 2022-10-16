// #include <OneWire.h>
// #include <DallasTemperature.h>
#include <string.h>
#include "shell.hpp"
#include "tgconn.hpp"

const int ledPin = 23;
bool ledState = LOW;

void processShellCmd();
void processTgEvent(const char *pEvent);

void setup() {
  gShell.setup(processShellCmd);
  gTgConn.setup(processTgEvent);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);
}

void loop() {
  gShell.process();
  gTgConn.process();
}

void processTgEvent(const char *pEvent) {
}

void processShellCmd(unsigned rdWordNum) {
  const char *rcmd = gShell.getWord(0);
  if (!strcmp("on", rcmd)) {
    digitalWrite(ledPin, LOW);
  } else if (!strcmp("off", rcmd)) {
    digitalWrite(ledPin, HIGH);
  // }
  //  else if (!strcmp("conn", rcmd)) {
  //   if (rdWordNum != 3) {
  //     gTgConn.connect_wifi();
  //   } else {
  //     gTgConn.connect_wifi(gShell.getWord(1), gShell.getWord(2));
  //   }
  } else if (!strcmp("dis", rcmd)) {
    gTgConn.disconnect();
  } else if (!strcmp("status", rcmd)) {
    gTgConn.reportStatus();
  // } else if (!strcmp("check", rcmd)) {
  //   gTgConn.check_messages();
  } else if (*rcmd) {
    Serial.print("Unknown command: ");
    Serial.println(rcmd);
  }
}

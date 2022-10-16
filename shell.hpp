#ifndef SHELL_H
#define SHELL_H

#include <Arduino.h>

#define NL "\r\n"

class Shell {
public:
  using commandCB_t = void(*)(unsigned rdWordNum);
  Shell();
  void setup(commandCB_t rfCommandCB);
  void process();
  const char *getInput();
  const char *getWord(unsigned rdIdx);
  void clear();

private:
  static constexpr uint32_t mdBaudRate = 115200;
  static constexpr unsigned mdMaxWordNum = 8;
  static constexpr char* maPrompt = "\e[93mesp> \e[0m";

  commandCB_t mfCommandCB = nullptr;
  char maBuf[128];
  char *maWord[mdMaxWordNum][2];
  char *mpNext = maBuf;
  char *mpLast = maBuf;
  bool mbRefresh = true;
  unsigned mdWordNum = 0;
};

extern Shell gShell;

#endif // SHELL_H
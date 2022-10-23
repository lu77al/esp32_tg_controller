#include <cctype>
#include "HardwareSerial.h"
#include "shell.hpp"

Shell gShell;

Shell::Shell() {
  maBuf[sizeof(maBuf)-1] = 0;
}

void Shell::setup(commandCB_t rfCommandCB) {
  Serial.begin(mdBaudRate);
  mfCommandCB = rfCommandCB;
}

void Shell::command(char *rpInput, char *rpEOI) {
  if (!rpEOI) {
    rpEOI = rpInput + strlen(rpInput);
  }
  while (rpEOI > rpInput && isspace(*(rpEOI - 1))) --rpEOI;
  *rpEOI = ' ';
  mdWordNum = 0;
  char *pCh = rpInput;
  while (pCh < rpEOI && mdWordNum < mdMaxWordNum) {
    while (isspace(*pCh)) ++pCh;
    maWord[mdWordNum][0] = pCh;
    while (!isspace(*pCh)) ++pCh;
    maWord[mdWordNum][1] = pCh;
    ++mdWordNum;
  }
  *rpEOI = 0;
  Serial.println("");
  mbRefresh = true;
  if (mfCommandCB && mdWordNum) {
    mfCommandCB(mdWordNum);
  }
}

void Shell::process() {
  mdWordNum = 0;
  if (mbRefresh) {
    Serial.print(maPrompt);
    *mpNext = 0;
    Serial.print(maBuf);
    mbRefresh = false;
  }
  int dNextChar;
  while ((dNextChar = Serial.read()) >= 0) {
    switch (dNextChar) {
      case 13:
      {
        command(maBuf, mpNext);
        mpNext = maBuf;
      }
      break;
      case 127:
        if (mpNext > maBuf) {
          --mpNext;
          Serial.print("\e[D \e[D");
        }
      break;
      default:
        *(mpNext++) = dNextChar;
        if (&maBuf[sizeof(maBuf)-1] == mpNext) {
          mpNext = maBuf;
        }
        Serial.write((char *)&dNextChar, 1);
    }
  }
}

const char *Shell::getInput() {
  return mdWordNum ? maWord[0][0] : "";
}

const char *Shell::getWord(unsigned rdIdx) {
  if (rdIdx < mdWordNum) {
    *(maWord[rdIdx][1]) = 0;
    return maWord[rdIdx][0];
  } else {
    return "";
  }
}

void Shell::clear() {
  if (!mbRefresh) {
    Serial.print("\e[2K\e[1G");
    mbRefresh = true;
  }
}

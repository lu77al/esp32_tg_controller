#include "tgconn.hpp"
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include "shell.hpp"

TgConn gTgConn;

namespace {

constexpr uint32_t mdINITIAL_DELAY_MS = 3000;
constexpr uint32_t mdWIFI_STATE_CHANGE_DELAY_MS = 2 * 1000;
constexpr uint32_t mdWIFI_RESERVE_NETWORK_TIME_MS = 30 * 60 * 1000;
constexpr uint32_t mdWIFI_CONNECTION_ATTEMPT_MS = 15 * 1000;
constexpr uint32_t mdWIFI_DISCONNECT_ATTEMPT_MS = 5 * 1000;
constexpr uint32_t mdTG_FAST_UPDATE_MS = 3 * 1000;
constexpr uint32_t mdTG_SLOW_UPDATE_MS = 15 * 1000;

constexpr uint32_t mdTG_FAILS_TO_RECONNECT = 5;
constexpr uint32_t mdTG_FAST_UPDATE_NUM = 10;

struct AccessPoint_t {
  const char* ssid;
  const char* password;
  bool reserve;
};

#if __has_include("credentials.hpp")
#include "credentials.hpp"
#else 
AccessPoint_t maAccessPoint[] = {
  {.ssid = "MAIN_AP",   .password = nullptr,            .reserve = false},
  {.ssid = "RESERV_AP", .password = "reserv_password",  .reserve = true},
};
#define AP_NUM (sizeof(maAccessPoint)/sizeof(AccessPoint_t))
#define BOT_TOKEN "BOT_TOKEN: ???????????????????????????????????"
#define CHAT_ID   "-CHAT_ID-"
#endif // __has_include("credentials.hpp")

const char *aWiFiStatus[] = {
    "IDLE_STATUS",
    "NO_SSID_AVAIL",
    "SCAN_COMPLETED",
    "CONNECTED",
    "CONNECT_FAILED",
    "CONNECTION_LOST",
    "DISCONNECTED",
};

#define WIFI_STATUS_NUM (sizeof(aWiFiStatus)/sizeof(aWiFiStatus[0]))

#ifdef ESP8266
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

wl_status_t meWiFiStatus = WL_NO_SHIELD;
unsigned long mdPowerUpTime;
unsigned long mdNextActionTime;
unsigned long mdSwitchNetworkTime;
unsigned mdApIdx = AP_NUM - 1;
unsigned mdNoResponceCnt = 0;
unsigned mdFastUpdateCnt = 0;

} // namespace

TgConn::TgConn() {
}

void TgConn::setup(eventCB_t rfEventCB) {
  mdNextActionTime = mdPowerUpTime = millis() + mdINITIAL_DELAY_MS;
  meWiFiStatus = WL_NO_SHIELD;
  #if defined(ESP8266)
    configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
    client.setTrustAnchors(&cert);    // Add root certificate for api.telegram.org
  #elif defined(ESP32)
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  #endif
}

void TgConn::process() {
  auto eWiFiStatus = WiFi.status();
  // Process WiFi state change. Log, reset counters
  if (eWiFiStatus != meWiFiStatus) {
    meWiFiStatus = eWiFiStatus;
    gShell.clear();
    printWiFiStatus(meWiFiStatus);
    unsigned long dNextActionTime = millis() + mdWIFI_STATE_CHANGE_DELAY_MS;
    if (mdNextActionTime < dNextActionTime) {
      mdNextActionTime = dNextActionTime;
    }
    if (WL_CONNECTED == meWiFiStatus) {
      mdNoResponceCnt = 0;
      mdSwitchNetworkTime = millis() + mdWIFI_RESERVE_NETWORK_TIME_MS;
    }
  }
  // Delay before next action
  if (millis() < mdNextActionTime)  {
    return;
  }
  if (WL_CONNECTED == meWiFiStatus && maAccessPoint[mdApIdx].reserve && millis() > mdSwitchNetworkTime) {
    gShell.clear();
    Serial.println("Reserve network expired");
    disconnect();
    return;
  }
  // Try to connect next AP form the list
  if (meWiFiStatus != WL_CONNECTED) {
    mdApIdx = (mdApIdx < AP_NUM - 1) ? (mdApIdx + 1) : 0;
    gShell.clear();
    Serial.printf("Connecting %s ..." NL, maAccessPoint[mdApIdx].ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(maAccessPoint[mdApIdx].ssid , maAccessPoint[mdApIdx].password);
    mdNextActionTime = millis() + mdWIFI_CONNECTION_ATTEMPT_MS;
    return;
  }
  // Check telegram income messages
  int dMsgAvailNum = bot.getUpdates(bot.last_message_received + 1);
  gShell.clear();
  Serial.printf("Check tg in (fast=%d fail=%d)" NL, mdFastUpdateCnt, mdNoResponceCnt);
  if (dMsgAvailNum > 0) { // Process messages, swithch to frequent update
    mdFastUpdateCnt = mdTG_FAST_UPDATE_NUM;
    mdNoResponceCnt = 0;
    for (int i=0; i<dMsgAvailNum; i++) {
      String tId = String(bot.messages[i].chat_id);
      String tText = String(bot.messages[i].text);
      String tName = String(bot.messages[i].from_name);
      Serial.printf("%s(%s)> %s" NL, tName.c_str(), tId.c_str(), tText.c_str());
    }
  } else if (UniversalTelegramBot::Result::Ok == bot._lastResult) { // Connection Ok, no messges 
    mdNoResponceCnt = 0;
    if (mdFastUpdateCnt) {
      --mdFastUpdateCnt;
    }
  } else { // Connection error, switch another network after some attempts
    ++mdNoResponceCnt;
    Serial.printf("No responce (%d)" NL, mdNoResponceCnt);
    if (mdNoResponceCnt > mdTG_FAILS_TO_RECONNECT) {
      disconnect();
      return;
    }
  }
  mdNextActionTime = millis() + (mdFastUpdateCnt ? mdTG_FAST_UPDATE_MS : mdTG_SLOW_UPDATE_MS); // Fast/slow update 
}

void TgConn::printWiFiStatus(int rdStatusIdx) {
  Serial.print("WiFi ");
  if (rdStatusIdx < WIFI_STATUS_NUM) {
    Serial.print(aWiFiStatus[rdStatusIdx]);
  } else {
    Serial.print(rdStatusIdx);
  }
  if (WL_CONNECTED == rdStatusIdx) {
    Serial.printf(" %s/%s", maAccessPoint[mdApIdx].ssid, WiFi.localIP().toString().c_str());
  }
  Serial.println("");
}

void TgConn::reportStatus() {
  auto eWiFiStatus = WiFi.status();
  meWiFiStatus = eWiFiStatus;
  gShell.clear();
  printWiFiStatus(meWiFiStatus);
}

void TgConn::disconnect() {
  gShell.clear();
  Serial.println("Disconnecting...");
  WiFi.disconnect(true, true);
  mdNextActionTime = millis() + mdWIFI_DISCONNECT_ATTEMPT_MS;
}

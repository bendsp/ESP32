#include "time_service.h"

#include <WiFi.h>

#include "clock_config.h"

namespace {

constexpr unsigned long kWifiConnectTimeoutMs = 20000;
constexpr unsigned long kWifiStatusIntervalMs = 500;
constexpr unsigned long kWifiRetryIntervalMs = 10000;
constexpr unsigned long kInitialTimeSyncTimeoutMs = 10000;
constexpr unsigned long kTimeSyncRetryIntervalMs = 60000;

bool isWifiConnectedNow() {
  return WiFi.status() == WL_CONNECTED;
}

bool connectToWifi(TimeServiceState& state) {
  if (state.wifiConnected) {
    return true;
  }

  unsigned long nowMs = millis();
  if (!state.wifiConnectionStarted) {
    if (state.lastWifiRetryMs != 0 && nowMs - state.lastWifiRetryMs < kWifiRetryIntervalMs) {
      return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    state.wifiConnectionStarted = true;
    state.wifiConnectStartMs = nowMs;
    state.lastWifiStatusMs = nowMs;

    Serial.print("Connecting to Wi-Fi SSID ");
    Serial.println(WIFI_SSID);
  }

  if (isWifiConnectedNow()) {
    state.wifiConnected = true;
    state.wifiConnectionStarted = false;

    Serial.println();
    Serial.println("Wi-Fi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    return true;
  }

  if (nowMs - state.lastWifiStatusMs >= kWifiStatusIntervalMs) {
    Serial.print(".");
    state.lastWifiStatusMs = nowMs;
  }

  if (nowMs - state.wifiConnectStartMs >= kWifiConnectTimeoutMs) {
    Serial.println();
    Serial.println("Wi-Fi connection failed");
    WiFi.disconnect(true, true);
    state.wifiConnected = false;
    state.wifiConnectionStarted = false;
    state.lastWifiRetryMs = nowMs;
    return false;
  }

  return false;
}

bool syncTimeFromInternet(TimeServiceState& state) {
  if (!isWifiConnectedNow()) {
    if (state.wifiConnected) {
      Serial.println("Wi-Fi lost");
    }
    state.wifiConnected = false;
    state.timeSynced = false;
    state.timeSyncStarted = false;
    return false;
  }

  unsigned long nowMs = millis();
  if (!state.timeSyncStarted) {
    state.timeSyncStarted = true;
    state.lastTimeSyncAttemptMs = nowMs;
    configTzTime(TIME_ZONE, NTP_SERVER_1, NTP_SERVER_2);
    Serial.println("Syncing time from NTP...");
  }

  tm timeInfo;
  if (getLocalTime(&timeInfo, 0)) {
    state.timeSynced = true;
    state.timeSyncStarted = false;
    Serial.print("Time synced: ");
    Serial.println(&timeInfo, "%H:%M:%S %d/%m/%Y");
    return true;
  }

  if (nowMs - state.lastTimeSyncAttemptMs >= kInitialTimeSyncTimeoutMs) {
    Serial.println("Initial NTP sync failed");
    state.timeSynced = false;
    state.timeSyncStarted = false;
    state.lastTimeSyncAttemptMs = nowMs;
  }

  return false;
}

}  // namespace

void initTimeServiceState(TimeServiceState& state) {
  state.wifiConnected = false;
  state.wifiConnectionStarted = false;
  state.timeSynced = false;
  state.timeSyncStarted = false;
  state.wifiConnectStartMs = 0;
  state.lastWifiStatusMs = 0;
  state.lastWifiRetryMs = 0;
  state.lastTimeSyncAttemptMs = 0;
}

void tickTimeService(TimeServiceState& state) {
  if (!isWifiConnectedNow()) {
    state.wifiConnected = false;
    state.timeSynced = false;
    connectToWifi(state);
    return;
  }

  if (!state.wifiConnected) {
    state.wifiConnected = true;
    state.wifiConnectionStarted = false;

    Serial.println();
    Serial.println("Wi-Fi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
  }

  if (state.timeSynced) {
    return;
  }

  unsigned long nowMs = millis();
  if (!state.timeSyncStarted && state.lastTimeSyncAttemptMs != 0 &&
      nowMs - state.lastTimeSyncAttemptMs < kTimeSyncRetryIntervalMs) {
    return;
  }

  syncTimeFromInternet(state);
}

bool readCurrentLocalTime(TimeServiceState& state, tm& timeInfo, uint32_t timeoutMs) {
  if (!getLocalTime(&timeInfo, timeoutMs)) {
    state.timeSynced = false;
    return false;
  }

  state.timeSynced = true;
  return true;
}

bool isTimeServiceWifiConnected(const TimeServiceState& state) {
  return state.wifiConnected;
}

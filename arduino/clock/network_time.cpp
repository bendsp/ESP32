#include "network_time.h"

#include <WiFi.h>

#include "clock_config.h"

#define WIFI_CONNECT_TIMEOUT_MS 20000
#define WIFI_STATUS_INTERVAL_MS 500
#define WIFI_RETRY_INTERVAL_MS 10000
#define INITIAL_TIME_SYNC_TIMEOUT_MS 10000
#define TIME_SYNC_RETRY_INTERVAL_MS 60000

void initNetworkTimeState(NetworkTimeState& state) {
  state.wifiConnected = false;
  state.wifiConnectionStarted = false;
  state.timeSynced = false;
  state.timeSyncStarted = false;
  state.wifiConnectStartMs = 0;
  state.lastWifiStatusMs = 0;
  state.lastWifiRetryMs = 0;
  state.lastTimeSyncAttemptMs = 0;
}

bool isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool connectToWifi(NetworkTimeState& state) {
  if (state.wifiConnected) {
    return true;
  }

  unsigned long nowMs = millis();
  if (!state.wifiConnectionStarted) {
    if (state.lastWifiRetryMs != 0 && nowMs - state.lastWifiRetryMs < WIFI_RETRY_INTERVAL_MS) {
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

  if (isWifiConnected()) {
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

  if (nowMs - state.lastWifiStatusMs >= WIFI_STATUS_INTERVAL_MS) {
    Serial.print(".");
    state.lastWifiStatusMs = nowMs;
  }

  if (nowMs - state.wifiConnectStartMs >= WIFI_CONNECT_TIMEOUT_MS) {
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

bool syncTimeFromInternet(NetworkTimeState& state) {
  if (!isWifiConnected()) {
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

  if (nowMs - state.lastTimeSyncAttemptMs >= INITIAL_TIME_SYNC_TIMEOUT_MS) {
    Serial.println("Initial NTP sync failed");
    state.timeSynced = false;
    state.timeSyncStarted = false;
    state.lastTimeSyncAttemptMs = nowMs;
  }

  return false;
}

void maintainTimeSync(NetworkTimeState& state) {
  if (!isWifiConnected()) {
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
      nowMs - state.lastTimeSyncAttemptMs < TIME_SYNC_RETRY_INTERVAL_MS) {
    return;
  }

  syncTimeFromInternet(state);
}

bool readCurrentLocalTime(NetworkTimeState& state, tm& timeInfo, uint32_t timeoutMs) {
  if (!getLocalTime(&timeInfo, timeoutMs)) {
    state.timeSynced = false;
    return false;
  }

  state.timeSynced = true;
  return true;
}

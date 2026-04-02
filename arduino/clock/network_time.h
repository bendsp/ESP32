#ifndef NETWORK_TIME_H
#define NETWORK_TIME_H

#include <Arduino.h>
#include <time.h>

struct NetworkTimeState {
  bool wifiConnected;
  bool wifiConnectionStarted;
  bool timeSynced;
  bool timeSyncStarted;
  unsigned long wifiConnectStartMs;
  unsigned long lastWifiStatusMs;
  unsigned long lastWifiRetryMs;
  unsigned long lastTimeSyncAttemptMs;
};

void initNetworkTimeState(NetworkTimeState& state);
bool isWifiConnected();
bool connectToWifi(NetworkTimeState& state);
bool syncTimeFromInternet(NetworkTimeState& state);
void maintainTimeSync(NetworkTimeState& state);
bool readCurrentLocalTime(NetworkTimeState& state, tm& timeInfo, uint32_t timeoutMs);

#endif

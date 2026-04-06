#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <Arduino.h>
#include <time.h>

struct TimeServiceState {
  bool wifiConnected;
  bool wifiConnectionStarted;
  bool timeSynced;
  bool timeSyncStarted;
  unsigned long wifiConnectStartMs;
  unsigned long lastWifiStatusMs;
  unsigned long lastWifiRetryMs;
  unsigned long lastTimeSyncAttemptMs;
};

void initTimeServiceState(TimeServiceState& state);
void tickTimeService(TimeServiceState& state);
bool readCurrentLocalTime(TimeServiceState& state, tm& timeInfo, uint32_t timeoutMs);
bool isTimeServiceWifiConnected(const TimeServiceState& state);

#endif

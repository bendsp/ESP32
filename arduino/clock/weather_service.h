#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <Arduino.h>

struct WeatherData {
  bool valid;
  int temperatureC;
  uint8_t weatherCode;
  bool isDay;
  unsigned long lastUpdateMs;
};

struct WeatherServiceState {
  WeatherData current;
  unsigned long lastRequestMs;
  unsigned long refreshIntervalMs;
};

void initWeatherServiceState(WeatherServiceState& state);
bool tickWeatherService(WeatherServiceState& state, bool wifiConnected, unsigned long nowMs);
const WeatherData* getCurrentWeather(const WeatherServiceState& state);

#endif

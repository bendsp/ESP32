#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>

struct WeatherData {
  bool valid;
  int temperatureC;
  uint8_t weatherCode;
  bool isDay;
  unsigned long lastUpdateMs;
};

struct WeatherState {
  WeatherData current;
  unsigned long lastRequestMs;
  unsigned long refreshIntervalMs;
};

void initWeatherState(WeatherState& state);
bool maintainWeather(WeatherState& state, bool wifiConnected, unsigned long nowMs);
const WeatherData* getCurrentWeather(const WeatherState& state);

#endif

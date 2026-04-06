#include "weather.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "clock_config.h"

namespace {

const char* CURRENT_OBJECT_MARKER = "\"current\":{";
const char* TEMPERATURE_FIELD = "\"temperature_2m\":";
const char* WEATHER_CODE_FIELD = "\"weather_code\":";
const char* IS_DAY_FIELD = "\"is_day\":";

bool parseFloatField(const char* text, const char* field, float& value) {
  const char* fieldStart = strstr(text, field);
  if (fieldStart == nullptr) {
    return false;
  }

  fieldStart += strlen(field);
  while (*fieldStart == ' ' || *fieldStart == '\t') {
    fieldStart++;
  }

  char* end = nullptr;
  value = strtof(fieldStart, &end);
  return end != fieldStart;
}

bool parseIntField(const char* text, const char* field, long& value) {
  const char* fieldStart = strstr(text, field);
  if (fieldStart == nullptr) {
    return false;
  }

  fieldStart += strlen(field);
  while (*fieldStart == ' ' || *fieldStart == '\t') {
    fieldStart++;
  }

  char* end = nullptr;
  value = strtol(fieldStart, &end, 10);
  return end != fieldStart;
}

bool parseWeatherResponse(const String& body, WeatherData& data) {
  const char* currentObject = strstr(body.c_str(), CURRENT_OBJECT_MARKER);
  if (currentObject == nullptr) {
    return false;
  }

  float temperature = 0.0f;
  long weatherCode = 0;
  long isDay = 0;
  if (!parseFloatField(currentObject, TEMPERATURE_FIELD, temperature) ||
      !parseIntField(currentObject, WEATHER_CODE_FIELD, weatherCode) ||
      !parseIntField(currentObject, IS_DAY_FIELD, isDay)) {
    return false;
  }

  data.valid = true;
  data.temperatureC = static_cast<int>(lroundf(temperature));
  data.weatherCode = static_cast<uint8_t>(weatherCode);
  data.isDay = isDay != 0;
  data.lastUpdateMs = millis();
  return true;
}

bool refreshWeatherNow(WeatherState& state, unsigned long nowMs) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=";
  url += String(WEATHER_LATITUDE, 4);
  url += "&longitude=";
  url += String(WEATHER_LONGITUDE, 4);
  url += "&current=temperature_2m,weather_code,is_day&temperature_unit=celsius&timezone=auto";

  Serial.println("Fetching weather...");

  if (!http.begin(client, url)) {
    Serial.println("Weather request setup failed");
    state.lastRequestMs = nowMs;
    return false;
  }

  http.setConnectTimeout(5000);
  http.setTimeout(5000);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("Weather request failed: ");
    Serial.println(httpCode);
    http.end();
    state.lastRequestMs = nowMs;
    return false;
  }

  WeatherData next = {};
  bool parsed = parseWeatherResponse(http.getString(), next);
  http.end();
  state.lastRequestMs = nowMs;

  if (!parsed) {
    Serial.println("Weather response parse failed");
    return false;
  }

  state.current = next;
  Serial.print("Weather updated: ");
  Serial.print(state.current.temperatureC);
  Serial.print("C code=");
  Serial.print(state.current.weatherCode);
  Serial.print(" day=");
  Serial.println(state.current.isDay ? 1 : 0);
  return true;
}

}  // namespace

void initWeatherState(WeatherState& state) {
  state.current.valid = false;
  state.current.temperatureC = 0;
  state.current.weatherCode = 0;
  state.current.isDay = true;
  state.current.lastUpdateMs = 0;
  state.lastRequestMs = 0;
  state.refreshIntervalMs = WEATHER_REFRESH_INTERVAL_MS;
}

bool maintainWeather(WeatherState& state, bool wifiConnected, unsigned long nowMs) {
  if (!wifiConnected) {
    return false;
  }

  if (state.lastRequestMs != 0 && nowMs - state.lastRequestMs < state.refreshIntervalMs) {
    return false;
  }

  return refreshWeatherNow(state, nowMs);
}

const WeatherData* getCurrentWeather(const WeatherState& state) {
  if (!state.current.valid) {
    return nullptr;
  }

  return &state.current;
}

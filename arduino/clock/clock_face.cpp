#include "clock_face.h"

#include <stdio.h>
#include <string.h>

#include "display_matrix.h"

namespace {

bool updateWeatherDisplay(ClockFaceState& state, const WeatherServiceState& weatherService) {
  if (state.weatherTempTextId == 0) {
    return false;
  }

  const WeatherData* weather = getCurrentWeather(weatherService);
  if (weather == nullptr) {
    return false;
  }

  char temperatureText[8];
  snprintf(temperatureText, sizeof(temperatureText), "%d\xF7", weather->temperatureC);

  bool changed = false;
  const TextElement* weatherElement = getTextElement(state.scene, state.weatherTempTextId);
  if (weatherElement != nullptr && strcmp(weatherElement->content, temperatureText) != 0) {
    setTextElementContent(state.scene, state.weatherTempTextId, temperatureText, nullptr);
    changed = true;
  }

  WeatherIconKind nextIconKind = weather_icons::iconKindForWeatherCode(weather->weatherCode, weather->isDay);
  if (!state.overlay.weatherIconVisible || state.overlay.weatherIconKind != nextIconKind) {
    state.overlay.weatherIconVisible = true;
    state.overlay.weatherIconKind = nextIconKind;
    changed = true;
  }

  return changed;
}

bool updateClockDisplayFromRtc(ClockFaceState& state, TimeServiceState& timeService) {
  if (state.dateTextId == 0 || state.hoursTextId == 0 || state.minutesTextId == 0) {
    return false;
  }

  tm timeInfo;
  if (!readCurrentLocalTime(timeService, timeInfo, 10)) {
    return false;
  }

  char hoursText[3];
  char minutesText[3];
  char dateText[12];
  snprintf(hoursText, sizeof(hoursText), "%02d", timeInfo.tm_hour);
  snprintf(minutesText, sizeof(minutesText), "%02d", timeInfo.tm_min);
  static const char* kMonthNames[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  snprintf(dateText, sizeof(dateText), "%s %d", kMonthNames[timeInfo.tm_mon], timeInfo.tm_mday);

  bool changed = false;
  const TextElement* dateElement = getTextElement(state.scene, state.dateTextId);
  if (dateElement != nullptr && strcmp(dateElement->content, dateText) != 0) {
    setTextElementContent(state.scene, state.dateTextId, dateText, nullptr);
    changed = true;
  }

  const TextElement* hoursElement = getTextElement(state.scene, state.hoursTextId);
  if (hoursElement != nullptr && strcmp(hoursElement->content, hoursText) != 0) {
    setTextElementContent(state.scene, state.hoursTextId, hoursText, nullptr);
    changed = true;
  }

  const TextElement* minutesElement = getTextElement(state.scene, state.minutesTextId);
  if (minutesElement != nullptr && strcmp(minutesElement->content, minutesText) != 0) {
    setTextElementContent(state.scene, state.minutesTextId, minutesText, nullptr);
    changed = true;
  }

  return changed;
}

}  // namespace

void initClockFace(ClockFaceState& state, MatrixPanel_I2S_DMA* matrix) {
  initScene(state.scene);
  clearSceneRenderOverlay(state.overlay);
  state.dateTextId = 0;
  state.hoursTextId = 0;
  state.minutesTextId = 0;
  state.weatherTempTextId = 0;
  state.dirty = true;

  if (matrix == nullptr) {
    return;
  }

  TextStyle dateStyle = defaultTextStyle();
  dateStyle.size = 1;
  TextMetrics dateMetrics = measureTextAt(matrix, "Apr 30", dateStyle.size, 1, 0);
  dateStyle.x = 1;
  dateStyle.y = 1 - dateMetrics.y1;
  state.dateTextId = createTextElement(state.scene, "___ __", dateStyle, nullptr);

  TextStyle weatherStyle = defaultTextStyle();
  weatherStyle.size = 1;
  TextMetrics weatherMetrics = measureTextAt(matrix, "--\xF7", weatherStyle.size, 0, 0);
  weatherStyle.x = kDisplayMatrixPanelWidth - 1 - static_cast<int16_t>(weatherMetrics.width) - weatherMetrics.x1;
  weatherStyle.y = 1 - weatherMetrics.y1;
  state.weatherTempTextId = createTextElement(state.scene, "--\xF7", weatherStyle, nullptr);
  state.overlay.weatherIconVisible = false;
  state.overlay.weatherIconKind = WEATHER_ICON_NONE;
  state.overlay.weatherIconX = weatherStyle.x - 9;
  state.overlay.weatherIconY = 1;

  TextStyle clockStyle = defaultTextStyle();
  clockStyle.size = 2;

  TextMetrics fullMetrics = measureTextAt(matrix, "XX:XX", clockStyle.size, 0, 0);
  TextMetrics hoursMetrics = measureTextAt(matrix, "XX", clockStyle.size, 0, 0);
  TextMetrics colonMetrics = measureTextAt(matrix, ":", clockStyle.size, 0, 0);

  int16_t groupX = (kDisplayMatrixPanelWidth - static_cast<int16_t>(fullMetrics.width)) / 2 - fullMetrics.x1;
  int16_t groupY = dateStyle.y + static_cast<int16_t>(dateMetrics.height) + 4 - fullMetrics.y1;

  TextStyle hoursStyle = clockStyle;
  hoursStyle.x = groupX;
  hoursStyle.y = groupY;
  state.hoursTextId = createTextElement(state.scene, "XX", hoursStyle, nullptr);

  TextStyle colonStyle = clockStyle;
  colonStyle.x = groupX + static_cast<int16_t>(hoursMetrics.width);
  colonStyle.y = groupY;
  colonStyle.blink = true;
  createTextElement(state.scene, ":", colonStyle, nullptr);

  TextStyle minutesStyle = clockStyle;
  minutesStyle.x = colonStyle.x + static_cast<int16_t>(colonMetrics.width);
  minutesStyle.y = groupY;
  state.minutesTextId = createTextElement(state.scene, "XX", minutesStyle, nullptr);
}

void tickClockFace(ClockFaceState& state, TimeServiceState& timeService, const WeatherServiceState& weatherService) {
  bool changed = false;
  changed = updateWeatherDisplay(state, weatherService) || changed;
  changed = updateClockDisplayFromRtc(state, timeService) || changed;
  state.dirty = state.dirty || changed;
}

bool clockFaceIsDirty(const ClockFaceState& state) {
  return state.dirty;
}

void clearClockFaceDirty(ClockFaceState& state) {
  state.dirty = false;
}

SceneState& clockFaceScene(ClockFaceState& state) {
  return state.scene;
}

const SceneState& clockFaceScene(const ClockFaceState& state) {
  return state.scene;
}

const SceneRenderOverlay* clockFaceOverlay(const ClockFaceState& state) {
  return &state.overlay;
}

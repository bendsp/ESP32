#include "clock_widgets.h"

#include <stdio.h>
#include <string.h>

namespace {

constexpr int16_t kWeatherIconWidth = 8;
constexpr int16_t kWeatherIconHeight = 8;
constexpr int16_t kWeatherIconGap = 1;
constexpr const char kDateSampleText[] = "Sep 30";
constexpr const char kDatePlaceholderText[] = "___ __";
constexpr const char kWeatherSampleText[] = "-12\xF7";
constexpr const char kWeatherPlaceholderText[] = "--\xF7";

int16_t alignedFrameX(const LayoutRect& contentRect, int16_t preferredWidth) {
  int16_t remaining = contentRect.width - preferredWidth;
  if (remaining < 0) {
    remaining = 0;
  }

  return contentRect.x + remaining / 2;
}

int16_t alignedTopCursorY(const LayoutRect& contentRect, const TextMetrics& metrics) {
  return contentRect.y - metrics.y1;
}

}  // namespace

void initDateWidget(DateWidgetState& widget, SceneState& scene) {
  widget.textId = 0;
  widget.style = defaultTextStyle();
  widget.style.size = 1;
  widget.metrics = {0, 0, 0, 0};
  widget.placement = {makeLayoutRect(0, 0, 0, 0), makeLayoutRect(0, 0, 0, 0)};

  widget.textId = createTextElement(scene, kDatePlaceholderText, widget.style, nullptr);
}

WidgetMetrics measureDateWidget(const DateWidgetState& widget, MatrixPanel_I2S_DMA* matrix) {
  WidgetMetrics metrics = {0, 0, 0, 0};
  if (matrix == nullptr) {
    return metrics;
  }

  TextMetrics sampleMetrics = measureTextAt(matrix, kDateSampleText, widget.style.size, 0, 0);
  metrics.preferredWidth = static_cast<int16_t>(sampleMetrics.width);
  metrics.preferredHeight = static_cast<int16_t>(sampleMetrics.height);
  metrics.minWidth = metrics.preferredWidth;
  metrics.minHeight = metrics.preferredHeight;
  return metrics;
}

void placeDateWidget(DateWidgetState& widget, SceneState& scene, MatrixPanel_I2S_DMA* matrix, const WidgetPlacement& placement) {
  if (matrix == nullptr || widget.textId == 0) {
    return;
  }

  widget.metrics = measureDateWidget(widget, matrix);
  widget.placement = placement;

  TextMetrics sampleMetrics = measureTextAt(matrix, kDateSampleText, widget.style.size, 0, 0);
  int16_t frameX = alignedFrameX(placement.contentRect, widget.metrics.preferredWidth);
  int16_t drawX = frameX - sampleMetrics.x1;
  int16_t drawY = alignedTopCursorY(placement.contentRect, sampleMetrics);
  setTextElementPosition(scene, widget.textId, drawX, drawY);
}

bool updateDateWidget(DateWidgetState& widget, SceneState& scene, TimeServiceState& timeService) {
  if (widget.textId == 0) {
    return false;
  }

  tm timeInfo;
  if (!readCurrentLocalTime(timeService, timeInfo, 10)) {
    return false;
  }

  char dateText[12];
  static const char* kMonthNames[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  snprintf(dateText, sizeof(dateText), "%s %d", kMonthNames[timeInfo.tm_mon], timeInfo.tm_mday);

  const TextElement* dateElement = getTextElement(scene, widget.textId);
  if (dateElement == nullptr || strcmp(dateElement->content, dateText) == 0) {
    return false;
  }

  return setTextElementContent(scene, widget.textId, dateText, nullptr);
}

void initWeatherWidget(WeatherWidgetState& widget, SceneState& scene) {
  widget.temperatureTextId = 0;
  widget.style = defaultTextStyle();
  widget.style.size = 1;
  widget.metrics = {0, 0, 0, 0};
  widget.placement = {makeLayoutRect(0, 0, 0, 0), makeLayoutRect(0, 0, 0, 0)};
  widget.iconX = 0;
  widget.iconY = 0;
  widget.iconKind = WEATHER_ICON_NONE;
  widget.iconVisible = false;

  widget.temperatureTextId = createTextElement(scene, kWeatherPlaceholderText, widget.style, nullptr);
}

WidgetMetrics measureWeatherWidget(const WeatherWidgetState& widget, MatrixPanel_I2S_DMA* matrix) {
  WidgetMetrics metrics = {0, 0, 0, 0};
  if (matrix == nullptr) {
    return metrics;
  }

  TextMetrics textMetrics = measureTextAt(matrix, kWeatherSampleText, widget.style.size, 0, 0);
  metrics.preferredWidth = kWeatherIconWidth + kWeatherIconGap + static_cast<int16_t>(textMetrics.width);
  int16_t textHeight = static_cast<int16_t>(textMetrics.height);
  metrics.preferredHeight = textHeight > kWeatherIconHeight ? textHeight : kWeatherIconHeight;
  metrics.minWidth = metrics.preferredWidth;
  metrics.minHeight = metrics.preferredHeight;
  return metrics;
}

void placeWeatherWidget(
    WeatherWidgetState& widget,
    SceneState& scene,
    MatrixPanel_I2S_DMA* matrix,
    const WidgetPlacement& placement) {
  if (matrix == nullptr || widget.temperatureTextId == 0) {
    return;
  }

  widget.metrics = measureWeatherWidget(widget, matrix);
  widget.placement = placement;

  TextMetrics sampleTextMetrics = measureTextAt(matrix, kWeatherSampleText, widget.style.size, 0, 0);
  int16_t clusterX = alignedFrameX(placement.contentRect, widget.metrics.preferredWidth);
  int16_t clusterY = placement.contentRect.y;

  widget.iconX = clusterX;
  widget.iconY = clusterY + (widget.metrics.preferredHeight - kWeatherIconHeight) / 2;

  int16_t textBoxX = clusterX + kWeatherIconWidth + kWeatherIconGap;
  int16_t textDrawX = textBoxX - sampleTextMetrics.x1;
  int16_t textDrawY = alignedTopCursorY(placement.contentRect, sampleTextMetrics);
  setTextElementPosition(scene, widget.temperatureTextId, textDrawX, textDrawY);
}

bool updateWeatherWidget(
    WeatherWidgetState& widget,
    SceneState& scene,
    const WeatherServiceState& weatherService,
    SceneRenderOverlay& overlay) {
  if (widget.temperatureTextId == 0) {
    return false;
  }

  const WeatherData* weather = getCurrentWeather(weatherService);
  if (weather == nullptr) {
    return false;
  }

  char temperatureText[8];
  snprintf(temperatureText, sizeof(temperatureText), "%d\xF7", weather->temperatureC);

  bool changed = false;
  const TextElement* weatherElement = getTextElement(scene, widget.temperatureTextId);
  if (weatherElement != nullptr && strcmp(weatherElement->content, temperatureText) != 0) {
    setTextElementContent(scene, widget.temperatureTextId, temperatureText, nullptr);
    changed = true;
  }

  WeatherIconKind nextIconKind = weather_icons::iconKindForWeatherCode(weather->weatherCode, weather->isDay);
  bool iconChanged = !widget.iconVisible || widget.iconKind != nextIconKind;
  widget.iconVisible = true;
  widget.iconKind = nextIconKind;

  if (!overlay.weatherIconVisible || overlay.weatherIconKind != widget.iconKind ||
      overlay.weatherIconX != widget.iconX || overlay.weatherIconY != widget.iconY) {
    overlay.weatherIconVisible = widget.iconVisible;
    overlay.weatherIconKind = widget.iconKind;
    overlay.weatherIconX = widget.iconX;
    overlay.weatherIconY = widget.iconY;
    changed = true;
  } else if (iconChanged) {
    changed = true;
  }

  return changed;
}

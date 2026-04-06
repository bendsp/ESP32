#include "clock_widgets.h"

#include <stdio.h>
#include <string.h>

namespace {

constexpr int16_t kWeatherIconWidth = 8;
constexpr int16_t kWeatherIconHeight = 8;
constexpr int16_t kWeatherIconGap = 1;
constexpr int16_t kDateMonthDayGap = 1;
constexpr const char kDateMonthSampleText[] = "Apr";
constexpr const char kDateDaySampleText[] = "30";
constexpr const char kDateMonthPlaceholderText[] = "___";
constexpr const char kDateDayPlaceholderText[] = "00";
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
  widget.monthTextId = 0;
  widget.dayTextId = 0;
  widget.style = defaultTextStyle();
  widget.style.size = 1;
  widget.metrics = {0, 0, 0, 0};
  widget.placement = {makeLayoutRect(0, 0, 0, 0), makeLayoutRect(0, 0, 0, 0)};

  widget.monthTextId = createTextElement(scene, kDateMonthPlaceholderText, widget.style, nullptr);
  widget.dayTextId = createTextElement(scene, kDateDayPlaceholderText, widget.style, nullptr);
}

WidgetMetrics measureDateWidget(const DateWidgetState& widget, MatrixPanel_I2S_DMA* matrix) {
  WidgetMetrics metrics = {0, 0, 0, 0};
  if (matrix == nullptr) {
    return metrics;
  }

  TextMetrics monthMetrics = measureTextAt(matrix, kDateMonthSampleText, widget.style.size, 0, 0);
  TextMetrics dayMetrics = measureTextAt(matrix, kDateDaySampleText, widget.style.size, 0, 0);
  metrics.preferredWidth =
      static_cast<int16_t>(monthMetrics.width) + kDateMonthDayGap + static_cast<int16_t>(dayMetrics.width);
  metrics.preferredHeight = static_cast<int16_t>(monthMetrics.height);
  if (static_cast<int16_t>(dayMetrics.height) > metrics.preferredHeight) {
    metrics.preferredHeight = static_cast<int16_t>(dayMetrics.height);
  }
  metrics.minWidth = metrics.preferredWidth;
  metrics.minHeight = metrics.preferredHeight;
  return metrics;
}

void placeDateWidget(DateWidgetState& widget, SceneState& scene, MatrixPanel_I2S_DMA* matrix, const WidgetPlacement& placement) {
  if (matrix == nullptr || widget.monthTextId == 0 || widget.dayTextId == 0) {
    return;
  }

  widget.metrics = measureDateWidget(widget, matrix);
  widget.placement = placement;

  TextMetrics monthMetrics = measureTextAt(matrix, kDateMonthSampleText, widget.style.size, 0, 0);
  TextMetrics dayMetrics = measureTextAt(matrix, kDateDaySampleText, widget.style.size, 0, 0);
  int16_t frameX = alignedFrameX(placement.slotRect, widget.metrics.preferredWidth);
  int16_t drawY = alignedTopCursorY(placement.contentRect, monthMetrics);
  int16_t monthDrawX = frameX - monthMetrics.x1;
  int16_t dayBoxX = frameX + static_cast<int16_t>(monthMetrics.width) + kDateMonthDayGap;
  int16_t dayDrawX = dayBoxX - dayMetrics.x1;
  setTextElementPosition(scene, widget.monthTextId, monthDrawX, drawY);
  setTextElementPosition(scene, widget.dayTextId, dayDrawX, drawY);
}

bool updateDateWidget(DateWidgetState& widget, SceneState& scene, TimeServiceState& timeService) {
  if (widget.monthTextId == 0 || widget.dayTextId == 0) {
    return false;
  }

  tm timeInfo;
  if (!readCurrentLocalTime(timeService, timeInfo, 10)) {
    return false;
  }

  char monthText[4];
  char dayText[3];
  static const char* kMonthNames[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  snprintf(monthText, sizeof(monthText), "%s", kMonthNames[timeInfo.tm_mon]);
  snprintf(dayText, sizeof(dayText), "%02d", timeInfo.tm_mday);

  bool changed = false;
  const TextElement* monthElement = getTextElement(scene, widget.monthTextId);
  if (monthElement != nullptr && strcmp(monthElement->content, monthText) != 0) {
    setTextElementContent(scene, widget.monthTextId, monthText, nullptr);
    changed = true;
  }

  const TextElement* dayElement = getTextElement(scene, widget.dayTextId);
  if (dayElement != nullptr && strcmp(dayElement->content, dayText) != 0) {
    setTextElementContent(scene, widget.dayTextId, dayText, nullptr);
    changed = true;
  }

  return changed;
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

void placeWeatherWidgetFromCurrentContent(
    WeatherWidgetState& widget,
    SceneState& scene,
    MatrixPanel_I2S_DMA* matrix) {
  if (matrix == nullptr || widget.temperatureTextId == 0 || widget.placement.slotRect.width <= 0 ||
      widget.placement.slotRect.height <= 0) {
    return;
  }

  const TextElement* currentElement = getTextElement(scene, widget.temperatureTextId);
  if (currentElement == nullptr) {
    return;
  }

  TextMetrics currentTextMetrics =
      measureTextAt(matrix, currentElement->content, widget.style.size, 0, 0);
  int16_t currentTextWidth = static_cast<int16_t>(currentTextMetrics.width);
  int16_t currentTextHeight = static_cast<int16_t>(currentTextMetrics.height);
  int16_t clusterHeight = currentTextHeight > kWeatherIconHeight ? currentTextHeight : kWeatherIconHeight;

  int16_t textBoxRight = widget.placement.slotRect.x + widget.placement.slotRect.width - 1;
  int16_t textBoxX = textBoxRight - currentTextWidth + 1;
  int16_t clusterY = widget.placement.slotRect.y;

  widget.iconX = textBoxX - kWeatherIconGap - kWeatherIconWidth;
  widget.iconY = clusterY + (clusterHeight - kWeatherIconHeight) / 2;

  int16_t textDrawX = textBoxX - currentTextMetrics.x1;
  int16_t textDrawY = alignedTopCursorY(widget.placement.slotRect, currentTextMetrics);
  setTextElementPosition(scene, widget.temperatureTextId, textDrawX, textDrawY);
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
  placeWeatherWidgetFromCurrentContent(widget, scene, matrix);
}

bool updateWeatherWidget(
    WeatherWidgetState& widget,
    SceneState& scene,
    MatrixPanel_I2S_DMA* matrix,
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

  placeWeatherWidgetFromCurrentContent(widget, scene, matrix);

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

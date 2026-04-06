#ifndef CLOCK_WIDGETS_H
#define CLOCK_WIDGETS_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "layout.h"
#include "scene_core.h"
#include "scene_render.h"
#include "time_service.h"
#include "weather_service.h"

struct DateWidgetState {
  uint16_t monthTextId;
  uint16_t dayTextId;
  TextStyle style;
  WidgetMetrics metrics;
  WidgetPlacement placement;
};

struct WeatherWidgetState {
  uint16_t temperatureTextId;
  TextStyle style;
  WidgetMetrics metrics;
  WidgetPlacement placement;
  int16_t iconX;
  int16_t iconY;
  WeatherIconKind iconKind;
  bool iconVisible;
};

void initDateWidget(DateWidgetState& widget, SceneState& scene);
WidgetMetrics measureDateWidget(const DateWidgetState& widget, MatrixPanel_I2S_DMA* matrix);
void placeDateWidget(DateWidgetState& widget, SceneState& scene, MatrixPanel_I2S_DMA* matrix, const WidgetPlacement& placement);
bool updateDateWidget(DateWidgetState& widget, SceneState& scene, TimeServiceState& timeService);

void initWeatherWidget(WeatherWidgetState& widget, SceneState& scene);
WidgetMetrics measureWeatherWidget(const WeatherWidgetState& widget, MatrixPanel_I2S_DMA* matrix);
void placeWeatherWidget(
    WeatherWidgetState& widget,
    SceneState& scene,
    MatrixPanel_I2S_DMA* matrix,
    const WidgetPlacement& placement);
bool updateWeatherWidget(
    WeatherWidgetState& widget,
    SceneState& scene,
    MatrixPanel_I2S_DMA* matrix,
    const WeatherServiceState& weatherService,
    SceneRenderOverlay& overlay);

#endif

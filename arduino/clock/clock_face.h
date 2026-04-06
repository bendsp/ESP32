#ifndef CLOCK_FACE_H
#define CLOCK_FACE_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "clock_widgets.h"
#include "layout.h"
#include "scene_core.h"
#include "scene_render.h"
#include "time_service.h"
#include "weather_service.h"

enum ClockTopWidget : uint8_t {
  CLOCK_TOP_WIDGET_DATE = 0,
  CLOCK_TOP_WIDGET_WEATHER = 1
};

struct ClockFaceArrangement {
  ClockTopWidget leftTopWidget;
  ClockTopWidget rightTopWidget;
};

struct ClockFaceState {
  SceneState scene;
  SceneRenderOverlay overlay;
  ClockFaceArrangement arrangement;
  DateWidgetState dateWidget;
  WeatherWidgetState weatherWidget;
  uint16_t hoursTextId;
  uint16_t colonTextId;
  uint16_t minutesTextId;
  bool dirty;
};

ClockFaceArrangement defaultClockFaceArrangement();
void initClockFace(ClockFaceState& state, MatrixPanel_I2S_DMA* matrix);
void applyClockFaceArrangement(ClockFaceState& state, MatrixPanel_I2S_DMA* matrix, const ClockFaceArrangement& arrangement);
void tickClockFace(ClockFaceState& state, TimeServiceState& timeService, const WeatherServiceState& weatherService);
bool clockFaceIsDirty(const ClockFaceState& state);
void clearClockFaceDirty(ClockFaceState& state);
SceneState& clockFaceScene(ClockFaceState& state);
const SceneState& clockFaceScene(const ClockFaceState& state);
const SceneRenderOverlay* clockFaceOverlay(const ClockFaceState& state);

#endif

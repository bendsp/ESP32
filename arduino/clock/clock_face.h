#ifndef CLOCK_FACE_H
#define CLOCK_FACE_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "scene_core.h"
#include "scene_render.h"
#include "time_service.h"
#include "weather_service.h"

struct ClockFaceState {
  SceneState scene;
  SceneRenderOverlay overlay;
  uint16_t dateTextId;
  uint16_t hoursTextId;
  uint16_t minutesTextId;
  uint16_t weatherTempTextId;
  bool dirty;
};

void initClockFace(ClockFaceState& state, MatrixPanel_I2S_DMA* matrix);
void tickClockFace(ClockFaceState& state, TimeServiceState& timeService, const WeatherServiceState& weatherService);
bool clockFaceIsDirty(const ClockFaceState& state);
void clearClockFaceDirty(ClockFaceState& state);
SceneState& clockFaceScene(ClockFaceState& state);
const SceneState& clockFaceScene(const ClockFaceState& state);
const SceneRenderOverlay* clockFaceOverlay(const ClockFaceState& state);

#endif

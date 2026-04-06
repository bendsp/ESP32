#ifndef SCENE_RENDER_H
#define SCENE_RENDER_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "display_matrix.h"
#include "layout.h"
#include "scene_core.h"
#include "weather_icons.h"

struct TextMetrics {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
};

struct SceneRenderOverlay {
  bool weatherIconVisible;
  WeatherIconKind weatherIconKind;
  int16_t weatherIconX;
  int16_t weatherIconY;
  bool debugCornersVisible;
  uint8_t debugRectCount;
  LayoutRect debugRects[3];
};

TextMetrics measureTextAt(MatrixPanel_I2S_DMA* matrix, const char* text, uint8_t size, int16_t x, int16_t y);
void clearSceneRenderOverlay(SceneRenderOverlay& overlay);
void renderScene(DisplayContext& display, const SceneState& scene, const SceneRenderOverlay* overlay);

#endif

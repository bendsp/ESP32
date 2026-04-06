#include "scene_render.h"

#include <math.h>

namespace {

uint16_t to565(MatrixPanel_I2S_DMA* matrix, uint8_t red, uint8_t green, uint8_t blue) {
  return matrix->color565(red, green, blue);
}

void clearScreen(MatrixPanel_I2S_DMA* matrix) {
  matrix->fillScreenRGB888(0, 0, 0);
}

void presentFrame(MatrixPanel_I2S_DMA* matrix) {
  matrix->flipDMABuffer();
}

TextMetrics getTextMetrics(MatrixPanel_I2S_DMA* matrix, const TextElement& element, int16_t x, int16_t y) {
  return measureTextAt(matrix, element.content, element.style.size, x, y);
}

void computeTextRenderPosition(
    const SceneState& scene,
    const TextElement& element,
    int16_t& drawX,
    int16_t& drawY) {
  if (!textElementHasZone(scene, element)) {
    drawX = element.style.x;
    drawY = element.style.y;
    return;
  }

  const Zone* zone = getZone(scene, element.zoneId);
  drawX = zone->x + element.style.x + static_cast<int16_t>(lroundf(element.scrollOffsetPx));
  drawY = zone->y + element.style.y;
}

void applyClipForText(DisplayMatrix* matrix, const SceneState& scene, const TextElement& element) {
  if (!textElementHasZone(scene, element)) {
    matrix->clearClipRect();
    return;
  }

  const Zone* zone = getZone(scene, element.zoneId);
  matrix->setClipRect(zone->x, zone->y, zone->width, zone->height);
}

void drawTextGlyphsAt(MatrixPanel_I2S_DMA* matrix, const TextElement& element, int16_t drawX, int16_t drawY) {
  if (element.style.backgroundBox) {
    TextMetrics metrics = measureTextAt(matrix, element.content, element.style.size, drawX, drawY);
    matrix->fillRect(metrics.x1, metrics.y1, metrics.width, metrics.height, to565(matrix, 0, 0, 0));
  }

  matrix->setTextColor(to565(matrix, element.style.red, element.style.green, element.style.blue));
  matrix->setCursor(drawX, drawY);
  matrix->print(element.content);
}

bool shouldDrawTextElement(const SceneState& scene, const TextElement& element, unsigned long nowMs) {
  if (!element.allocated || !element.style.visible || element.content[0] == '\0') {
    return false;
  }

  if (element.style.blink && scene.blinkIntervalMs != 0 &&
      ((nowMs / scene.blinkIntervalMs) % 2UL) != 0) {
    return false;
  }

  return true;
}

bool shouldDrawLoopFollower(MatrixPanel_I2S_DMA* matrix, const SceneState& scene, const TextElement& element) {
  if (!element.scrollEnabled || element.scrollMode != SCROLL_MODE_LOOP || element.content[0] == '\0' ||
      !textElementHasZone(scene, element)) {
    return false;
  }

  const Zone* zone = getZone(scene, element.zoneId);
  TextMetrics localMetrics = measureTextAt(matrix, element.content, element.style.size, element.style.x, element.style.y);
  return static_cast<int16_t>(localMetrics.width) > zone->width;
}

void drawTextElement(DisplayMatrix* matrix, const SceneState& scene, const TextElement& element, unsigned long nowMs) {
  if (!shouldDrawTextElement(scene, element, nowMs)) {
    return;
  }

  int16_t drawX = 0;
  int16_t drawY = 0;
  computeTextRenderPosition(scene, element, drawX, drawY);

  matrix->setTextWrap(false);
  matrix->setTextSize(element.style.size);
  applyClipForText(matrix, scene, element);
  drawTextGlyphsAt(matrix, element, drawX, drawY);

  if (shouldDrawLoopFollower(matrix, scene, element)) {
    TextMetrics localMetrics = getTextMetrics(matrix, element, element.style.x, element.style.y);
    int16_t followerDrawX =
        drawX + static_cast<int16_t>(localMetrics.width) + static_cast<int16_t>(element.scrollGapPx);
    drawTextGlyphsAt(matrix, element, followerDrawX, drawY);
  }

  matrix->clearClipRect();
}

}  // namespace

TextMetrics measureTextAt(MatrixPanel_I2S_DMA* matrix, const char* text, uint8_t size, int16_t x, int16_t y) {
  TextMetrics metrics = {x, y, 0, 0};
  matrix->setTextWrap(false);
  matrix->setTextSize(size);
  matrix->getTextBounds(text, x, y, &metrics.x1, &metrics.y1, &metrics.width, &metrics.height);
  return metrics;
}

void clearSceneRenderOverlay(SceneRenderOverlay& overlay) {
  overlay.weatherIconVisible = false;
  overlay.weatherIconKind = WEATHER_ICON_NONE;
  overlay.weatherIconX = 0;
  overlay.weatherIconY = 0;
}

void renderScene(DisplayContext& display, const SceneState& scene, const SceneRenderOverlay* overlay) {
  DisplayMatrix* matrix = clippedMatrix(display);
  if (matrix == nullptr) {
    return;
  }

  clearScreen(matrix);
  unsigned long nowMs = millis();

  int lastDrawOrder = -1;
  while (true) {
    int index = findNextDrawOrderElement(scene, lastDrawOrder);
    if (index < 0) {
      break;
    }

    drawTextElement(matrix, scene, scene.textElements[index], nowMs);
    lastDrawOrder = scene.textElements[index].drawOrder;
  }

  if (overlay != nullptr && overlay->weatherIconVisible && overlay->weatherIconKind != WEATHER_ICON_NONE) {
    weather_icons::drawWeatherIcon(matrix, overlay->weatherIconKind, overlay->weatherIconX, overlay->weatherIconY);
  }

  presentFrame(matrix);
}

#include "scene_anim.h"

#include <math.h>

#include "scene_render.h"

namespace {

bool updateTextScrollState(SceneState& scene, TextElement& element, MatrixPanel_I2S_DMA* matrix, unsigned long nowMs) {
  if (!element.allocated || !element.scrollEnabled || !textElementHasZone(scene, element) ||
      element.content[0] == '\0') {
    bool changed = fabsf(element.scrollOffsetPx) > 0.01f || element.scrollPauseUntilMs != 0;
    if (changed) {
      resetTextAnimationState(element);
      element.lastScrollUpdateMs = nowMs;
    }
    return changed;
  }

  const Zone* zone = getZone(scene, element.zoneId);
  TextMetrics localMetrics = measureTextAt(matrix, element.content, element.style.size, element.style.x, element.style.y);
  int16_t textLeftBase = localMetrics.x1;
  int16_t textWidth = static_cast<int16_t>(localMetrics.width);

  if (textWidth <= zone->width) {
    bool changed = fabsf(element.scrollOffsetPx) > 0.01f || element.scrollPauseUntilMs != 0;
    if (changed) {
      resetTextAnimationState(element);
      element.lastScrollUpdateMs = nowMs;
    }
    return changed;
  }

  if (element.lastScrollUpdateMs == 0) {
    element.lastScrollUpdateMs = nowMs;
    if (element.scrollDirection == 0) {
      element.scrollDirection = -1;
    }
    return false;
  }

  if (element.scrollPauseUntilMs > nowMs) {
    element.lastScrollUpdateMs = nowMs;
    return false;
  }

  float deltaSeconds = static_cast<float>(nowMs - element.lastScrollUpdateMs) / 1000.0f;
  element.lastScrollUpdateMs = nowMs;
  if (deltaSeconds <= 0.0f) {
    return false;
  }

  float oldOffset = element.scrollOffsetPx;
  float newOffset = element.scrollOffsetPx +
                    static_cast<float>(element.scrollDirection) *
                        static_cast<float>(element.scrollSpeedPxPerSec) * deltaSeconds;

  if (element.scrollMode == SCROLL_MODE_LOOP) {
    float period = static_cast<float>(textWidth + static_cast<int16_t>(element.scrollGapPx));
    while (newOffset <= -period) {
      newOffset += period;
    }
    while (newOffset > 0.0f) {
      newOffset -= period;
    }
    element.scrollOffsetPx = newOffset;
    return fabsf(oldOffset - element.scrollOffsetPx) > 0.01f;
  }

  float maxOffset = static_cast<float>(-textLeftBase);
  float minOffset = static_cast<float>(zone->width - (textLeftBase + textWidth));

  if (newOffset <= minOffset) {
    newOffset = minOffset;
    element.scrollDirection = 1;
    element.scrollPauseUntilMs = nowMs + element.scrollPauseMs;
  } else if (newOffset >= maxOffset) {
    newOffset = maxOffset;
    element.scrollDirection = -1;
    element.scrollPauseUntilMs = nowMs + element.scrollPauseMs;
  }

  element.scrollOffsetPx = newOffset;
  return fabsf(oldOffset - element.scrollOffsetPx) > 0.01f;
}

}  // namespace

void resetTextAnimationState(TextElement& element) {
  element.scrollOffsetPx = 0.0f;
  element.scrollDirection = -1;
  element.scrollPauseUntilMs = 0;
  element.lastScrollUpdateMs = 0;
}

bool blinkPhaseIsVisible(const SceneState& scene, unsigned long nowMs) {
  if (scene.blinkIntervalMs == 0) {
    return true;
  }

  return ((nowMs / scene.blinkIntervalMs) % 2UL) == 0;
}

bool updateSceneAnimations(SceneState& scene, MatrixPanel_I2S_DMA* matrix, unsigned long nowMs) {
  if (matrix == nullptr) {
    return false;
  }

  bool changed = false;
  bool currentBlinkVisible = blinkPhaseIsVisible(scene, nowMs);
  if (sceneHasBlinkingText(scene) && currentBlinkVisible != scene.lastBlinkVisible) {
    changed = true;
  }
  scene.lastBlinkVisible = currentBlinkVisible;

  for (uint8_t i = 0; i < kSceneMaxTextElements; ++i) {
    changed = updateTextScrollState(scene, scene.textElements[i], matrix, nowMs) || changed;
  }

  return changed;
}

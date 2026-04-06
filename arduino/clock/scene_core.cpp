#include "scene_core.h"

#include <string.h>

namespace {

void resetZone(Zone& zone) {
  zone.allocated = false;
  zone.id = 0;
  zone.x = 0;
  zone.y = 0;
  zone.width = 0;
  zone.height = 0;
}

void resetTextAnimation(TextElement& element) {
  element.scrollOffsetPx = 0.0f;
  element.scrollDirection = -1;
  element.scrollPauseUntilMs = 0;
  element.lastScrollUpdateMs = 0;
}

void resetTextElement(TextElement& element) {
  element.allocated = false;
  element.id = 0;
  element.drawOrder = 0;
  element.style = defaultTextStyle();
  element.content[0] = '\0';
  element.zoneId = 0;
  element.scrollEnabled = false;
  element.scrollMode = SCROLL_MODE_BOUNCE;
  element.scrollSpeedPxPerSec = kSceneDefaultScrollSpeedPxPerSec;
  element.scrollPauseMs = kSceneDefaultScrollPauseMs;
  element.scrollGapPx = kSceneDefaultScrollGapPx;
  resetTextAnimation(element);
}

void copyTextContent(char* destination, const char* source, size_t capacity, bool* truncated) {
  if (truncated != nullptr) {
    *truncated = false;
  }

  if (capacity == 0) {
    return;
  }

  if (source == nullptr) {
    destination[0] = '\0';
    return;
  }

  size_t sourceLength = strlen(source);
  if (sourceLength >= capacity && truncated != nullptr) {
    *truncated = true;
  }

  strncpy(destination, source, capacity - 1);
  destination[capacity - 1] = '\0';
}

int findFreeZoneSlot(const SceneState& scene) {
  for (uint8_t i = 0; i < kSceneMaxZones; ++i) {
    if (!scene.zones[i].allocated) {
      return i;
    }
  }

  return -1;
}

int findFreeTextElementSlot(const SceneState& scene) {
  for (uint8_t i = 0; i < kSceneMaxTextElements; ++i) {
    if (!scene.textElements[i].allocated) {
      return i;
    }
  }

  return -1;
}

bool hasTextUsingZone(const SceneState& scene, uint16_t zoneId) {
  for (uint8_t i = 0; i < kSceneMaxTextElements; ++i) {
    if (scene.textElements[i].allocated && scene.textElements[i].zoneId == zoneId) {
      return true;
    }
  }

  return false;
}

}  // namespace

TextStyle defaultTextStyle() {
  TextStyle style = {0, 0, 1, 255, 255, 255, false, true, false};
  return style;
}

void initScene(SceneState& scene) {
  scene.blinkIntervalMs = kSceneDefaultBlinkIntervalMs;
  scene.lastBlinkVisible = true;
  scene.nextZoneId = 1;
  scene.nextTextElementId = 1;
  scene.nextDrawOrder = 1;

  for (uint8_t i = 0; i < kSceneMaxZones; ++i) {
    resetZone(scene.zones[i]);
  }

  for (uint8_t i = 0; i < kSceneMaxTextElements; ++i) {
    resetTextElement(scene.textElements[i]);
  }
}

void clearScene(SceneState& scene) {
  initScene(scene);
}

Zone* findZoneById(SceneState& scene, uint16_t id) {
  for (uint8_t i = 0; i < kSceneMaxZones; ++i) {
    if (scene.zones[i].allocated && scene.zones[i].id == id) {
      return &scene.zones[i];
    }
  }

  return nullptr;
}

const Zone* getZone(const SceneState& scene, uint16_t id) {
  for (uint8_t i = 0; i < kSceneMaxZones; ++i) {
    if (scene.zones[i].allocated && scene.zones[i].id == id) {
      return &scene.zones[i];
    }
  }

  return nullptr;
}

uint16_t createZone(SceneState& scene, int16_t x, int16_t y, int16_t width, int16_t height) {
  int slot = findFreeZoneSlot(scene);
  if (slot < 0) {
    return 0;
  }

  Zone& zone = scene.zones[slot];
  zone.allocated = true;
  zone.id = scene.nextZoneId++;
  zone.x = x;
  zone.y = y;
  zone.width = width;
  zone.height = height;
  return zone.id;
}

bool deleteZone(SceneState& scene, uint16_t id) {
  Zone* zone = findZoneById(scene, id);
  if (zone == nullptr || hasTextUsingZone(scene, id)) {
    return false;
  }

  resetZone(*zone);
  return true;
}

TextElement* findTextElementById(SceneState& scene, uint16_t id) {
  for (uint8_t i = 0; i < kSceneMaxTextElements; ++i) {
    if (scene.textElements[i].allocated && scene.textElements[i].id == id) {
      return &scene.textElements[i];
    }
  }

  return nullptr;
}

const TextElement* getTextElement(const SceneState& scene, uint16_t id) {
  for (uint8_t i = 0; i < kSceneMaxTextElements; ++i) {
    if (scene.textElements[i].allocated && scene.textElements[i].id == id) {
      return &scene.textElements[i];
    }
  }

  return nullptr;
}

uint16_t createTextElement(SceneState& scene, const char* content, const TextStyle& style, bool* truncated) {
  int slot = findFreeTextElementSlot(scene);
  if (slot < 0) {
    return 0;
  }

  TextElement& element = scene.textElements[slot];
  element.allocated = true;
  element.id = scene.nextTextElementId++;
  element.drawOrder = scene.nextDrawOrder++;
  element.style = style;
  copyTextContent(element.content, content, sizeof(element.content), truncated);
  element.zoneId = 0;
  element.scrollEnabled = false;
  element.scrollMode = SCROLL_MODE_BOUNCE;
  element.scrollSpeedPxPerSec = kSceneDefaultScrollSpeedPxPerSec;
  element.scrollPauseMs = kSceneDefaultScrollPauseMs;
  element.scrollGapPx = kSceneDefaultScrollGapPx;
  resetTextAnimation(element);
  return element.id;
}

bool deleteTextElement(SceneState& scene, uint16_t id) {
  TextElement* element = findTextElementById(scene, id);
  if (element == nullptr) {
    return false;
  }

  resetTextElement(*element);
  return true;
}

bool setTextElementContent(SceneState& scene, uint16_t id, const char* content, bool* truncated) {
  TextElement* element = findTextElementById(scene, id);
  if (element == nullptr) {
    return false;
  }

  copyTextContent(element->content, content, sizeof(element->content), truncated);
  resetTextAnimation(*element);
  return true;
}

bool assignTextElementZone(SceneState& scene, uint16_t elementId, uint16_t zoneId) {
  TextElement* element = findTextElementById(scene, elementId);
  if (element == nullptr) {
    return false;
  }

  if (zoneId != 0 && getZone(scene, zoneId) == nullptr) {
    return false;
  }

  element->zoneId = zoneId;
  resetTextAnimation(*element);
  return true;
}

bool enableTextScroll(SceneState& scene, uint16_t elementId, bool enabled) {
  TextElement* element = findTextElementById(scene, elementId);
  if (element == nullptr) {
    return false;
  }

  element->scrollEnabled = enabled;
  resetTextAnimation(*element);
  return true;
}

bool setTextElementScrollMode(SceneState& scene, uint16_t elementId, ScrollMode mode) {
  TextElement* element = findTextElementById(scene, elementId);
  if (element == nullptr) {
    return false;
  }

  element->scrollMode = mode;
  resetTextAnimation(*element);
  return true;
}

bool setTextElementScrollSpeed(SceneState& scene, uint16_t elementId, uint16_t pixelsPerSecond) {
  TextElement* element = findTextElementById(scene, elementId);
  if (element == nullptr || pixelsPerSecond == 0) {
    return false;
  }

  element->scrollSpeedPxPerSec = pixelsPerSecond;
  resetTextAnimation(*element);
  return true;
}

bool setTextElementScrollPause(SceneState& scene, uint16_t elementId, unsigned long pauseMs) {
  TextElement* element = findTextElementById(scene, elementId);
  if (element == nullptr) {
    return false;
  }

  element->scrollPauseMs = pauseMs;
  resetTextAnimation(*element);
  return true;
}

bool setTextElementScrollGap(SceneState& scene, uint16_t elementId, uint16_t pixels) {
  TextElement* element = findTextElementById(scene, elementId);
  if (element == nullptr) {
    return false;
  }

  element->scrollGapPx = pixels;
  resetTextAnimation(*element);
  return true;
}

bool sceneHasBlinkingText(const SceneState& scene) {
  for (uint8_t i = 0; i < kSceneMaxTextElements; ++i) {
    if (scene.textElements[i].allocated && scene.textElements[i].style.blink) {
      return true;
    }
  }

  return false;
}

bool textElementHasZone(const SceneState& scene, const TextElement& element) {
  return element.zoneId != 0 && getZone(scene, element.zoneId) != nullptr;
}

int findNextDrawOrderElement(const SceneState& scene, int lastDrawOrder) {
  int bestIndex = -1;
  int bestDrawOrder = 0x7fffffff;

  for (uint8_t i = 0; i < kSceneMaxTextElements; ++i) {
    const TextElement& element = scene.textElements[i];
    if (!element.allocated || element.drawOrder <= lastDrawOrder) {
      continue;
    }

    if (element.drawOrder < bestDrawOrder) {
      bestDrawOrder = element.drawOrder;
      bestIndex = i;
    }
  }

  return bestIndex;
}

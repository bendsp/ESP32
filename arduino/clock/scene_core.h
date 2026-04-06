#ifndef SCENE_CORE_H
#define SCENE_CORE_H

#include <stddef.h>
#include <stdint.h>

inline constexpr uint8_t kSceneMaxTextElements = 16;
inline constexpr uint8_t kSceneMaxZones = 8;
inline constexpr size_t kSceneTextContentCapacity = 32;
inline constexpr unsigned long kSceneDefaultBlinkIntervalMs = 1000;
inline constexpr uint16_t kSceneDefaultScrollSpeedPxPerSec = 8;
inline constexpr unsigned long kSceneDefaultScrollPauseMs = 1500;
inline constexpr uint16_t kSceneDefaultScrollGapPx = 8;

enum ScrollMode : uint8_t {
  SCROLL_MODE_BOUNCE = 0,
  SCROLL_MODE_LOOP = 1
};

struct TextStyle {
  int16_t x;
  int16_t y;
  uint8_t size;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  bool backgroundBox;
  bool visible;
  bool blink;
};

struct Zone {
  bool allocated;
  uint16_t id;
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
};

struct TextElement {
  bool allocated;
  uint16_t id;
  uint16_t drawOrder;
  TextStyle style;
  char content[kSceneTextContentCapacity];
  uint16_t zoneId;
  bool scrollEnabled;
  ScrollMode scrollMode;
  uint16_t scrollSpeedPxPerSec;
  unsigned long scrollPauseMs;
  uint16_t scrollGapPx;
  float scrollOffsetPx;
  int8_t scrollDirection;
  unsigned long scrollPauseUntilMs;
  unsigned long lastScrollUpdateMs;
};

struct SceneState {
  unsigned long blinkIntervalMs;
  bool lastBlinkVisible;
  Zone zones[kSceneMaxZones];
  TextElement textElements[kSceneMaxTextElements];
  uint16_t nextZoneId;
  uint16_t nextTextElementId;
  uint16_t nextDrawOrder;
};

TextStyle defaultTextStyle();
void initScene(SceneState& scene);
void clearScene(SceneState& scene);

Zone* findZoneById(SceneState& scene, uint16_t id);
const Zone* getZone(const SceneState& scene, uint16_t id);
uint16_t createZone(SceneState& scene, int16_t x, int16_t y, int16_t width, int16_t height);
bool deleteZone(SceneState& scene, uint16_t id);

TextElement* findTextElementById(SceneState& scene, uint16_t id);
const TextElement* getTextElement(const SceneState& scene, uint16_t id);
uint16_t createTextElement(SceneState& scene, const char* content, const TextStyle& style, bool* truncated);
bool deleteTextElement(SceneState& scene, uint16_t id);
bool setTextElementContent(SceneState& scene, uint16_t id, const char* content, bool* truncated);
bool assignTextElementZone(SceneState& scene, uint16_t elementId, uint16_t zoneId);
bool enableTextScroll(SceneState& scene, uint16_t elementId, bool enabled);
bool setTextElementScrollMode(SceneState& scene, uint16_t elementId, ScrollMode mode);
bool setTextElementScrollSpeed(SceneState& scene, uint16_t elementId, uint16_t pixelsPerSecond);
bool setTextElementScrollPause(SceneState& scene, uint16_t elementId, unsigned long pauseMs);
bool setTextElementScrollGap(SceneState& scene, uint16_t elementId, uint16_t pixels);

bool sceneHasBlinkingText(const SceneState& scene);
bool textElementHasZone(const SceneState& scene, const TextElement& element);
int findNextDrawOrderElement(const SceneState& scene, int lastDrawOrder);

#endif

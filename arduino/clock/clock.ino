#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>
#include "clock_config.h"
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64
#define PANELS_NUMBER 1
#define PANEL_BRIGHTNESS 32
#define MATRIX_DRIVER HUB75_I2S_CFG::FM6126A
#define MATRIX_DRIVER_NAME "FM6126A"
#define MATRIX_CLKPHASE false
#define MATRIX_LAT_BLANKING 1

#define R1_PIN 37
#define G1_PIN 6
#define B1_PIN 36
#define R2_PIN 35
#define G2_PIN 5
#define B2_PIN 0
#define A_PIN 45
#define B_PIN 1
#define C_PIN 48
#define D_PIN 2
#define E_PIN 4
#define CLK_PIN 47
#define LAT_PIN 38
#define OE_PIN 21

#define MAX_TEXT_ELEMENTS 16
#define MAX_ZONES 8
#define TEXT_CONTENT_CAPACITY 32
#define SERIAL_LINE_CAPACITY 128
#define DEFAULT_BLINK_INTERVAL_MS 1000
#define DEFAULT_SCROLL_SPEED_PX_PER_SEC 10
#define DEFAULT_SCROLL_PAUSE_MS 1000
#define DEFAULT_SCROLL_GAP_PX 8
#define WIFI_CONNECT_TIMEOUT_MS 20000
#define WIFI_STATUS_INTERVAL_MS 500

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
  char content[TEXT_CONTENT_CAPACITY];
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
  uint8_t brightness;
  unsigned long blinkIntervalMs;
  bool lastBlinkVisible;
  Zone zones[MAX_ZONES];
  TextElement textElements[MAX_TEXT_ELEMENTS];
  uint16_t nextZoneId;
  uint16_t nextTextElementId;
  uint16_t nextDrawOrder;
};

struct TextMetrics {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
};

class ClippedMatrixPanel : public MatrixPanel_I2S_DMA {
public:
  explicit ClippedMatrixPanel(const HUB75_I2S_CFG& config)
      : MatrixPanel_I2S_DMA(config), clipEnabled(false), clipX(0), clipY(0), clipW(0), clipH(0) {}

  void setClipRect(int16_t x, int16_t y, int16_t w, int16_t h) {
    if (w <= 0 || h <= 0) {
      clipEnabled = false;
      return;
    }

    clipEnabled = true;
    clipX = x;
    clipY = y;
    clipW = w;
    clipH = h;
  }

  void clearClipRect() {
    clipEnabled = false;
  }

  virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (!isPointVisible(x, y)) {
      return;
    }

    MatrixPanel_I2S_DMA::drawPixel(x, y, color);
  }

  virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override {
    if (!clipLineHorizontal(x, y, w)) {
      return;
    }

    MatrixPanel_I2S_DMA::drawFastHLine(x, y, w, color);
  }

  virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override {
    if (!clipLineVertical(x, y, h)) {
      return;
    }

    MatrixPanel_I2S_DMA::drawFastVLine(x, y, h, color);
  }

  virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
    if (!clipRect(x, y, w, h)) {
      return;
    }

    MatrixPanel_I2S_DMA::fillRect(x, y, w, h, color);
  }

private:
  bool clipEnabled;
  int16_t clipX;
  int16_t clipY;
  int16_t clipW;
  int16_t clipH;

  bool isPointVisible(int16_t x, int16_t y) const {
    if (!clipEnabled) {
      return true;
    }

    return x >= clipX && x < clipX + clipW && y >= clipY && y < clipY + clipH;
  }

  bool clipLineHorizontal(int16_t& x, int16_t y, int16_t& w) const {
    if (w <= 0) {
      return false;
    }

    if (!clipEnabled) {
      return true;
    }

    if (y < clipY || y >= clipY + clipH) {
      return false;
    }

    int16_t xEnd = x + w;
    if (x < clipX) {
      x = clipX;
    }
    if (xEnd > clipX + clipW) {
      xEnd = clipX + clipW;
    }

    w = xEnd - x;
    return w > 0;
  }

  bool clipLineVertical(int16_t x, int16_t& y, int16_t& h) const {
    if (h <= 0) {
      return false;
    }

    if (!clipEnabled) {
      return true;
    }

    if (x < clipX || x >= clipX + clipW) {
      return false;
    }

    int16_t yEnd = y + h;
    if (y < clipY) {
      y = clipY;
    }
    if (yEnd > clipY + clipH) {
      yEnd = clipY + clipH;
    }

    h = yEnd - y;
    return h > 0;
  }

  bool clipRect(int16_t& x, int16_t& y, int16_t& w, int16_t& h) const {
    if (w <= 0 || h <= 0) {
      return false;
    }

    if (!clipEnabled) {
      return true;
    }

    int16_t xEnd = x + w;
    int16_t yEnd = y + h;

    if (x < clipX) {
      x = clipX;
    }
    if (y < clipY) {
      y = clipY;
    }
    if (xEnd > clipX + clipW) {
      xEnd = clipX + clipW;
    }
    if (yEnd > clipY + clipH) {
      yEnd = clipY + clipH;
    }

    w = xEnd - x;
    h = yEnd - y;
    return w > 0 && h > 0;
  }
};

ClippedMatrixPanel* matrix = nullptr;
SceneState scene;
char serialLineBuffer[SERIAL_LINE_CAPACITY];
size_t serialLineLength = 0;

TextStyle defaultTextStyle() {
  TextStyle style = {0, 0, 1, 255, 255, 255, false, true, false};
  return style;
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

bool parseInt32(const char* text, long& value) {
  if (text == nullptr || text[0] == '\0') {
    return false;
  }

  char* end = nullptr;
  value = strtol(text, &end, 10);
  return end != text && *end == '\0';
}

bool parseUInt16Value(const char* text, uint16_t& value) {
  long parsed;
  if (!parseInt32(text, parsed) || parsed < 0 || parsed > 65535) {
    return false;
  }

  value = static_cast<uint16_t>(parsed);
  return true;
}

bool parseRgb(const char* text, int& red, int& green, int& blue) {
  if (text == nullptr) {
    return false;
  }

  const char* firstComma = strchr(text, ',');
  const char* secondComma = (firstComma == nullptr) ? nullptr : strchr(firstComma + 1, ',');
  if (firstComma == nullptr || secondComma == nullptr || strchr(secondComma + 1, ',') != nullptr) {
    return false;
  }

  char redBuffer[8];
  char greenBuffer[8];
  char blueBuffer[8];
  size_t redLength = static_cast<size_t>(firstComma - text);
  size_t greenLength = static_cast<size_t>(secondComma - firstComma - 1);
  size_t blueLength = strlen(secondComma + 1);

  if (redLength == 0 || greenLength == 0 || blueLength == 0) {
    return false;
  }

  if (redLength >= sizeof(redBuffer) || greenLength >= sizeof(greenBuffer) || blueLength >= sizeof(blueBuffer)) {
    return false;
  }

  memcpy(redBuffer, text, redLength);
  redBuffer[redLength] = '\0';
  memcpy(greenBuffer, firstComma + 1, greenLength);
  greenBuffer[greenLength] = '\0';
  memcpy(blueBuffer, secondComma + 1, blueLength);
  blueBuffer[blueLength] = '\0';

  long redValue;
  long greenValue;
  long blueValue;
  if (!parseInt32(redBuffer, redValue) || !parseInt32(greenBuffer, greenValue) || !parseInt32(blueBuffer, blueValue)) {
    return false;
  }

  if (redValue < 0 || redValue > 255 || greenValue < 0 || greenValue > 255 || blueValue < 0 || blueValue > 255) {
    return false;
  }

  red = static_cast<int>(redValue);
  green = static_cast<int>(greenValue);
  blue = static_cast<int>(blueValue);
  return true;
}

bool parseBool01(const char* text, int& value) {
  long parsed;
  if (!parseInt32(text, parsed) || (parsed != 0 && parsed != 1)) {
    return false;
  }

  value = static_cast<int>(parsed);
  return true;
}

bool parseScrollMode(const char* text, ScrollMode& mode) {
  if (strcmp(text, "bounce") == 0) {
    mode = SCROLL_MODE_BOUNCE;
    return true;
  }

  if (strcmp(text, "loop") == 0) {
    mode = SCROLL_MODE_LOOP;
    return true;
  }

  return false;
}

const char* scrollModeName(ScrollMode mode) {
  return mode == SCROLL_MODE_LOOP ? "loop" : "bounce";
}

uint16_t to565(uint8_t red, uint8_t green, uint8_t blue) {
  return matrix->color565(red, green, blue);
}

TextMetrics measureTextAt(const char* text, uint8_t size, int16_t x, int16_t y) {
  TextMetrics metrics = {x, y, 0, 0};
  matrix->setTextWrap(false);
  matrix->setTextSize(size);
  matrix->getTextBounds(text, x, y, &metrics.x1, &metrics.y1, &metrics.width, &metrics.height);
  return metrics;
}

void clearScreen() {
  matrix->fillScreenRGB888(0, 0, 0);
}

void presentFrame() {
  matrix->flipDMABuffer();
}

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
  element.scrollSpeedPxPerSec = DEFAULT_SCROLL_SPEED_PX_PER_SEC;
  element.scrollPauseMs = DEFAULT_SCROLL_PAUSE_MS;
  element.scrollGapPx = DEFAULT_SCROLL_GAP_PX;
  resetTextAnimation(element);
}

void initScene() {
  scene.brightness = PANEL_BRIGHTNESS;
  scene.blinkIntervalMs = DEFAULT_BLINK_INTERVAL_MS;
  scene.lastBlinkVisible = true;
  scene.nextZoneId = 1;
  scene.nextTextElementId = 1;
  scene.nextDrawOrder = 1;

  for (uint8_t i = 0; i < MAX_ZONES; i++) {
    resetZone(scene.zones[i]);
  }

  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
    resetTextElement(scene.textElements[i]);
  }
}

void clearScene() {
  scene.nextZoneId = 1;
  scene.nextTextElementId = 1;
  scene.nextDrawOrder = 1;

  for (uint8_t i = 0; i < MAX_ZONES; i++) {
    resetZone(scene.zones[i]);
  }

  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
    resetTextElement(scene.textElements[i]);
  }
}

bool beginMatrix() {
  HUB75_I2S_CFG::i2s_pins pins = {
    R1_PIN, G1_PIN, B1_PIN,
    R2_PIN, G2_PIN, B2_PIN,
    A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
    LAT_PIN, OE_PIN, CLK_PIN
  };

  HUB75_I2S_CFG config(
      PANEL_WIDTH,
      PANEL_HEIGHT,
      PANELS_NUMBER,
      pins,
      MATRIX_DRIVER,
      HUB75_I2S_CFG::TYPE138,
      true);

  config.mx_height = PANEL_HEIGHT;
  config.chain_length = PANELS_NUMBER;
  config.gpio.e = E_PIN;
  config.clkphase = MATRIX_CLKPHASE;

  matrix = new ClippedMatrixPanel(config);
  matrix->setBrightness8(scene.brightness);
  if (!matrix->begin()) {
    return false;
  }

  matrix->setLatBlanking(MATRIX_LAT_BLANKING);
  return true;
}

Zone* findZoneById(uint16_t id) {
  for (uint8_t i = 0; i < MAX_ZONES; i++) {
    if (scene.zones[i].allocated && scene.zones[i].id == id) {
      return &scene.zones[i];
    }
  }

  return nullptr;
}

const Zone* getZone(uint16_t id) {
  return findZoneById(id);
}

int findFreeZoneSlot() {
  for (uint8_t i = 0; i < MAX_ZONES; i++) {
    if (!scene.zones[i].allocated) {
      return i;
    }
  }

  return -1;
}

bool hasTextUsingZone(uint16_t zoneId) {
  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
    if (scene.textElements[i].allocated && scene.textElements[i].zoneId == zoneId) {
      return true;
    }
  }

  return false;
}

uint16_t createZone(int16_t x, int16_t y, int16_t width, int16_t height) {
  int slot = findFreeZoneSlot();
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

bool deleteZone(uint16_t id) {
  Zone* zone = findZoneById(id);
  if (zone == nullptr) {
    return false;
  }

  if (hasTextUsingZone(id)) {
    Serial.print("Zone still in use: ");
    Serial.println(id);
    return false;
  }

  resetZone(*zone);
  return true;
}

bool setZoneField(uint16_t id, const char* field, const char* value) {
  Zone* zone = findZoneById(id);
  if (zone == nullptr) {
    Serial.print("Zone not found: ");
    Serial.println(id);
    return false;
  }

  long parsed;
  if (strcmp(field, "x") == 0) {
    if (!parseInt32(value, parsed) || parsed < INT16_MIN || parsed > INT16_MAX) {
      Serial.println("Invalid zone x. Use a signed 16-bit integer");
      return false;
    }
    zone->x = static_cast<int16_t>(parsed);
    return true;
  }

  if (strcmp(field, "y") == 0) {
    if (!parseInt32(value, parsed) || parsed < INT16_MIN || parsed > INT16_MAX) {
      Serial.println("Invalid zone y. Use a signed 16-bit integer");
      return false;
    }
    zone->y = static_cast<int16_t>(parsed);
    return true;
  }

  if (strcmp(field, "w") == 0) {
    if (!parseInt32(value, parsed) || parsed < 1 || parsed > INT16_MAX) {
      Serial.println("Invalid zone width. Use an integer from 1 to 32767");
      return false;
    }
    zone->width = static_cast<int16_t>(parsed);
    return true;
  }

  if (strcmp(field, "h") == 0) {
    if (!parseInt32(value, parsed) || parsed < 1 || parsed > INT16_MAX) {
      Serial.println("Invalid zone height. Use an integer from 1 to 32767");
      return false;
    }
    zone->height = static_cast<int16_t>(parsed);
    return true;
  }

  Serial.print("Unknown zone field: ");
  Serial.println(field);
  return false;
}

TextElement* findTextElementById(uint16_t id) {
  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
    if (scene.textElements[i].allocated && scene.textElements[i].id == id) {
      return &scene.textElements[i];
    }
  }

  return nullptr;
}

const TextElement* getTextElement(uint16_t id) {
  return findTextElementById(id);
}

int findFreeTextElementSlot() {
  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
    if (!scene.textElements[i].allocated) {
      return i;
    }
  }

  return -1;
}

uint16_t createTextElement(const char* content, const TextStyle& style, bool* truncated) {
  int slot = findFreeTextElementSlot();
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
  element.scrollSpeedPxPerSec = DEFAULT_SCROLL_SPEED_PX_PER_SEC;
  element.scrollPauseMs = DEFAULT_SCROLL_PAUSE_MS;
  element.scrollGapPx = DEFAULT_SCROLL_GAP_PX;
  resetTextAnimation(element);
  return element.id;
}

bool deleteTextElement(uint16_t id) {
  TextElement* element = findTextElementById(id);
  if (element == nullptr) {
    return false;
  }

  resetTextElement(*element);
  return true;
}

bool setTextElementContent(uint16_t id, const char* content, bool* truncated) {
  TextElement* element = findTextElementById(id);
  if (element == nullptr) {
    return false;
  }

  copyTextContent(element->content, content, sizeof(element->content), truncated);
  resetTextAnimation(*element);
  return true;
}

bool blinkPhaseIsVisible(unsigned long nowMs) {
  if (scene.blinkIntervalMs == 0) {
    return true;
  }

  return ((nowMs / scene.blinkIntervalMs) % 2UL) == 0;
}

bool hasBlinkingText() {
  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
    if (scene.textElements[i].allocated && scene.textElements[i].style.blink) {
      return true;
    }
  }

  return false;
}

TextMetrics getTextMetrics(const TextElement& element, int16_t x, int16_t y) {
  return measureTextAt(element.content, element.style.size, x, y);
}

bool textHasZone(const TextElement& element) {
  return element.zoneId != 0 && getZone(element.zoneId) != nullptr;
}

void computeTextRenderPosition(const TextElement& element, int16_t& drawX, int16_t& drawY) {
  if (!textHasZone(element)) {
    drawX = element.style.x;
    drawY = element.style.y;
    return;
  }

  const Zone* zone = getZone(element.zoneId);
  drawX = zone->x + element.style.x + static_cast<int16_t>(lroundf(element.scrollOffsetPx));
  drawY = zone->y + element.style.y;
}

void applyClipForText(const TextElement& element) {
  if (!textHasZone(element)) {
    matrix->clearClipRect();
    return;
  }

  const Zone* zone = getZone(element.zoneId);
  matrix->setClipRect(zone->x, zone->y, zone->width, zone->height);
}

void clearClipForText() {
  matrix->clearClipRect();
}

bool shouldDrawTextElement(const TextElement& element, unsigned long nowMs) {
  if (!element.allocated || !element.style.visible || element.content[0] == '\0') {
    return false;
  }

  if (element.style.blink && !blinkPhaseIsVisible(nowMs)) {
    return false;
  }

  return true;
}

bool shouldDrawLoopFollower(const TextElement& element, const Zone& zone) {
  if (!element.scrollEnabled || element.scrollMode != SCROLL_MODE_LOOP || element.content[0] == '\0') {
    return false;
  }

  TextMetrics localMetrics = getTextMetrics(element, element.style.x, element.style.y);
  return static_cast<int16_t>(localMetrics.width) > zone.width;
}

void drawTextGlyphsAt(const TextElement& element, int16_t drawX, int16_t drawY) {
  if (element.style.backgroundBox) {
    TextMetrics metrics = getTextMetrics(element, drawX, drawY);
    matrix->fillRect(metrics.x1, metrics.y1, metrics.width, metrics.height, to565(0, 0, 0));
  }

  matrix->setTextColor(to565(element.style.red, element.style.green, element.style.blue));
  matrix->setCursor(drawX, drawY);
  matrix->print(element.content);
}

bool updateTextScrollState(TextElement& element, unsigned long nowMs) {
  if (!element.allocated || !element.scrollEnabled || !textHasZone(element) || element.content[0] == '\0') {
    bool changed = fabsf(element.scrollOffsetPx) > 0.01f || element.scrollPauseUntilMs != 0;
    if (changed) {
      resetTextAnimation(element);
      element.lastScrollUpdateMs = nowMs;
    }
    return changed;
  }

  const Zone* zone = getZone(element.zoneId);
  TextMetrics localMetrics = getTextMetrics(element, element.style.x, element.style.y);
  int16_t textLeftBase = localMetrics.x1;
  int16_t textWidth = static_cast<int16_t>(localMetrics.width);

  if (textWidth <= zone->width) {
    bool changed = fabsf(element.scrollOffsetPx) > 0.01f || element.scrollPauseUntilMs != 0;
    if (changed) {
      resetTextAnimation(element);
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
  float newOffset = element.scrollOffsetPx + static_cast<float>(element.scrollDirection) * static_cast<float>(element.scrollSpeedPxPerSec) * deltaSeconds;

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

bool updateAnimatedScene(unsigned long nowMs) {
  bool changed = false;
  bool currentBlinkVisible = blinkPhaseIsVisible(nowMs);
  if (hasBlinkingText() && currentBlinkVisible != scene.lastBlinkVisible) {
    changed = true;
  }
  scene.lastBlinkVisible = currentBlinkVisible;

  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
    changed = updateTextScrollState(scene.textElements[i], nowMs) || changed;
  }

  return changed;
}

void drawTextElement(const TextElement& element, unsigned long nowMs) {
  if (!shouldDrawTextElement(element, nowMs)) {
    return;
  }

  int16_t drawX;
  int16_t drawY;
  computeTextRenderPosition(element, drawX, drawY);

  matrix->setTextWrap(false);
  matrix->setTextSize(element.style.size);
  applyClipForText(element);
  drawTextGlyphsAt(element, drawX, drawY);

  if (textHasZone(element)) {
    const Zone* zone = getZone(element.zoneId);
    if (zone != nullptr && shouldDrawLoopFollower(element, *zone)) {
      TextMetrics localMetrics = getTextMetrics(element, element.style.x, element.style.y);
      int16_t followerDrawX = drawX + static_cast<int16_t>(localMetrics.width) + static_cast<int16_t>(element.scrollGapPx);
      drawTextGlyphsAt(element, followerDrawX, drawY);
    }
  }

  clearClipForText();
}

int findNextDrawOrderElement(int lastDrawOrder) {
  int bestIndex = -1;
  int bestDrawOrder = INT_MAX;

  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
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

void renderScene() {
  clearScreen();
  unsigned long nowMs = millis();

  int lastDrawOrder = -1;
  while (true) {
    int index = findNextDrawOrderElement(lastDrawOrder);
    if (index < 0) {
      break;
    }

    drawTextElement(scene.textElements[index], nowMs);
    lastDrawOrder = scene.textElements[index].drawOrder;
  }

  presentFrame();
}

void printZone(const Zone& zone) {
  Serial.print("id=");
  Serial.print(zone.id);
  Serial.print(" x=");
  Serial.print(zone.x);
  Serial.print(" y=");
  Serial.print(zone.y);
  Serial.print(" w=");
  Serial.print(zone.width);
  Serial.print(" h=");
  Serial.println(zone.height);
}

void printTextElement(const TextElement& element) {
  Serial.print("id=");
  Serial.print(element.id);
  Serial.print(" order=");
  Serial.print(element.drawOrder);
  Serial.print(" x=");
  Serial.print(element.style.x);
  Serial.print(" y=");
  Serial.print(element.style.y);
  Serial.print(" size=");
  Serial.print(element.style.size);
  Serial.print(" rgb=");
  Serial.print(element.style.red);
  Serial.print(",");
  Serial.print(element.style.green);
  Serial.print(",");
  Serial.print(element.style.blue);
  Serial.print(" bg=");
  Serial.print(element.style.backgroundBox ? 1 : 0);
  Serial.print(" vis=");
  Serial.print(element.style.visible ? 1 : 0);
  Serial.print(" blink=");
  Serial.print(element.style.blink ? 1 : 0);
  Serial.print(" zone=");
  Serial.print(element.zoneId);
  Serial.print(" scroll=");
  Serial.print(element.scrollEnabled ? 1 : 0);
  Serial.print(" scroll_mode=");
  Serial.print(scrollModeName(element.scrollMode));
  Serial.print(" scroll_speed=");
  Serial.print(element.scrollSpeedPxPerSec);
  Serial.print(" scroll_pause=");
  Serial.print(element.scrollPauseMs);
  Serial.print(" scroll_gap=");
  Serial.print(element.scrollGapPx);
  Serial.print(" text=\"");
  Serial.print(element.content);
  Serial.println("\"");
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  help");
  Serial.println("  b <0-255>");
  Serial.println("  blink <milliseconds>");
  Serial.println("  clear");
  Serial.println("  list");
  Serial.println("  get <id>");
  Serial.println("  ta <x> <y> <size> <r,g,b> <bg> \"text\"");
  Serial.println("  tc \"text\"");
  Serial.println("  td <id>");
  Serial.println("  ts <id> <field> <value>");
  Serial.println("  za <x> <y> <w> <h>");
  Serial.println("  zd <zoneId>");
  Serial.println("  zg <zoneId>");
  Serial.println("  zlist");
  Serial.println("  zs <zoneId> <field> <value>");
  Serial.println("Text fields: x, y, size, rgb, bg, vis, blink, text, zone, scroll, scroll_mode, scroll_speed, scroll_pause, scroll_gap");
  Serial.println("Zone fields: x, y, w, h");
  Serial.println("Examples:");
  Serial.println("  za 0 56 64 8");
  Serial.println("  ts 4 zone 1");
  Serial.println("  ts 4 scroll 1");
  Serial.println("  ts 4 scroll_mode bounce");
  Serial.println("  ts 4 scroll_pause 1000");
  Serial.println("  ts 4 scroll_mode loop");
  Serial.println("  ts 4 scroll_gap 8");
}

bool isWifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi SSID ");
  Serial.println(WIFI_SSID);

  unsigned long startMs = millis();
  unsigned long lastStatusMs = 0;

  while (!isWifiConnected() && millis() - startMs < WIFI_CONNECT_TIMEOUT_MS) {
    unsigned long nowMs = millis();
    if (nowMs - lastStatusMs >= WIFI_STATUS_INTERVAL_MS) {
      Serial.print(".");
      lastStatusMs = nowMs;
    }
    delay(50);
  }

  Serial.println();

  if (!isWifiConnected()) {
    Serial.println("Wi-Fi connection failed");
    return false;
  }

  Serial.println("Wi-Fi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
  return true;
}

bool setBrightnessLevel(int value) {
  if (value < 0 || value > 255) {
    Serial.println("Brightness out of range. Use 0 to 255");
    return false;
  }

  scene.brightness = static_cast<uint8_t>(value);
  matrix->setBrightness8(scene.brightness);
  Serial.print("Brightness set to ");
  Serial.println(scene.brightness);
  return true;
}

bool setBlinkInterval(unsigned long intervalMs) {
  if (intervalMs == 0) {
    Serial.println("Blink interval must be at least 1 millisecond");
    return false;
  }

  scene.blinkIntervalMs = intervalMs;
  scene.lastBlinkVisible = blinkPhaseIsVisible(millis());
  Serial.print("Blink interval set to ");
  Serial.print(scene.blinkIntervalMs);
  Serial.println(" ms");
  return true;
}

bool setTextElementField(uint16_t id, const char* field, const char* value) {
  TextElement* element = findTextElementById(id);
  if (element == nullptr) {
    Serial.print("Text element not found: ");
    Serial.println(id);
    return false;
  }

  long parsed;
  if (strcmp(field, "x") == 0) {
    if (!parseInt32(value, parsed) || parsed < INT16_MIN || parsed > INT16_MAX) {
      Serial.println("Invalid x. Use a signed 16-bit integer");
      return false;
    }
    element->style.x = static_cast<int16_t>(parsed);
    resetTextAnimation(*element);
    return true;
  }

  if (strcmp(field, "y") == 0) {
    if (!parseInt32(value, parsed) || parsed < INT16_MIN || parsed > INT16_MAX) {
      Serial.println("Invalid y. Use a signed 16-bit integer");
      return false;
    }
    element->style.y = static_cast<int16_t>(parsed);
    resetTextAnimation(*element);
    return true;
  }

  if (strcmp(field, "size") == 0) {
    if (!parseInt32(value, parsed) || parsed < 1 || parsed > 255) {
      Serial.println("Invalid size. Use an integer from 1 to 255");
      return false;
    }
    element->style.size = static_cast<uint8_t>(parsed);
    resetTextAnimation(*element);
    return true;
  }

  if (strcmp(field, "rgb") == 0) {
    int red;
    int green;
    int blue;
    if (!parseRgb(value, red, green, blue)) {
      Serial.println("Invalid rgb. Use r,g,b with values from 0 to 255");
      return false;
    }
    element->style.red = static_cast<uint8_t>(red);
    element->style.green = static_cast<uint8_t>(green);
    element->style.blue = static_cast<uint8_t>(blue);
    return true;
  }

  if (strcmp(field, "bg") == 0) {
    int boolValue;
    if (!parseBool01(value, boolValue)) {
      Serial.println("Invalid bg. Use 0 or 1");
      return false;
    }
    element->style.backgroundBox = (boolValue == 1);
    return true;
  }

  if (strcmp(field, "vis") == 0) {
    int boolValue;
    if (!parseBool01(value, boolValue)) {
      Serial.println("Invalid vis. Use 0 or 1");
      return false;
    }
    element->style.visible = (boolValue == 1);
    return true;
  }

  if (strcmp(field, "blink") == 0) {
    int boolValue;
    if (!parseBool01(value, boolValue)) {
      Serial.println("Invalid blink. Use 0 or 1");
      return false;
    }
    element->style.blink = (boolValue == 1);
    return true;
  }

  if (strcmp(field, "text") == 0) {
    bool truncated = false;
    if (!setTextElementContent(id, value, &truncated)) {
      return false;
    }
    if (truncated) {
      Serial.print("Text truncated to ");
      Serial.print(TEXT_CONTENT_CAPACITY - 1);
      Serial.println(" characters");
    }
    return true;
  }

  if (strcmp(field, "zone") == 0) {
    uint16_t zoneId;
    if (!parseUInt16Value(value, zoneId)) {
      Serial.println("Invalid zone id");
      return false;
    }
    if (zoneId != 0 && getZone(zoneId) == nullptr) {
      Serial.print("Zone not found: ");
      Serial.println(zoneId);
      return false;
    }
    element->zoneId = zoneId;
    resetTextAnimation(*element);
    return true;
  }

  if (strcmp(field, "scroll") == 0) {
    int boolValue;
    if (!parseBool01(value, boolValue)) {
      Serial.println("Invalid scroll. Use 0 or 1");
      return false;
    }
    element->scrollEnabled = (boolValue == 1);
    resetTextAnimation(*element);
    return true;
  }

  if (strcmp(field, "scroll_mode") == 0) {
    ScrollMode mode;
    if (!parseScrollMode(value, mode)) {
      Serial.println("Invalid scroll_mode. Use bounce or loop");
      return false;
    }
    element->scrollMode = mode;
    resetTextAnimation(*element);
    return true;
  }

  if (strcmp(field, "scroll_speed") == 0) {
    if (!parseInt32(value, parsed) || parsed < 1 || parsed > 65535) {
      Serial.println("Invalid scroll_speed. Use an integer from 1 to 65535");
      return false;
    }
    element->scrollSpeedPxPerSec = static_cast<uint16_t>(parsed);
    resetTextAnimation(*element);
    return true;
  }

  if (strcmp(field, "scroll_pause") == 0) {
    if (!parseInt32(value, parsed) || parsed < 0) {
      Serial.println("Invalid scroll_pause. Use an integer from 0 and up");
      return false;
    }
    element->scrollPauseMs = static_cast<unsigned long>(parsed);
    resetTextAnimation(*element);
    return true;
  }

  if (strcmp(field, "scroll_gap") == 0) {
    if (!parseInt32(value, parsed) || parsed < 0 || parsed > 65535) {
      Serial.println("Invalid scroll_gap. Use an integer from 0 to 65535");
      return false;
    }
    element->scrollGapPx = static_cast<uint16_t>(parsed);
    resetTextAnimation(*element);
    return true;
  }

  Serial.print("Unknown field: ");
  Serial.println(field);
  return false;
}

int tokenizeCommand(char* line, char* tokens[], int maxTokens, bool& unterminatedQuote) {
  unterminatedQuote = false;
  int count = 0;
  char* cursor = line;

  while (*cursor != '\0') {
    while (*cursor != '\0' && isspace(static_cast<unsigned char>(*cursor))) {
      cursor++;
    }

    if (*cursor == '\0') {
      break;
    }

    if (count >= maxTokens) {
      return -1;
    }

    if (*cursor == '"') {
      cursor++;
      tokens[count++] = cursor;
      while (*cursor != '\0' && *cursor != '"') {
        cursor++;
      }
      if (*cursor != '"') {
        unterminatedQuote = true;
        return count;
      }
      *cursor = '\0';
      cursor++;
      continue;
    }

    tokens[count++] = cursor;
    while (*cursor != '\0' && !isspace(static_cast<unsigned char>(*cursor))) {
      cursor++;
    }
    if (*cursor == '\0') {
      break;
    }
    *cursor = '\0';
    cursor++;
  }

  return count;
}

void handleHelpCommand(int tokenCount) {
  if (tokenCount != 1) {
    Serial.println("Usage: help");
    return;
  }
  printHelp();
}

void handleBrightnessCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 2) {
    Serial.println("Usage: b <0-255>");
    return;
  }

  long value;
  if (!parseInt32(tokens[1], value)) {
    Serial.println("Invalid brightness. Use an integer from 0 to 255");
    return;
  }

  if (setBrightnessLevel(static_cast<int>(value))) {
    renderScene();
  }
}

void handleBlinkCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 2) {
    Serial.println("Usage: blink <milliseconds>");
    return;
  }

  long value;
  if (!parseInt32(tokens[1], value) || value < 1) {
    Serial.println("Invalid blink interval. Use an integer from 1 and up");
    return;
  }

  if (setBlinkInterval(static_cast<unsigned long>(value))) {
    renderScene();
  }
}

void handleClearCommand(int tokenCount) {
  if (tokenCount != 1) {
    Serial.println("Usage: clear");
    return;
  }

  clearScene();
  Serial.println("Scene cleared");
  renderScene();
}

void handleListCommand(int tokenCount) {
  if (tokenCount != 1) {
    Serial.println("Usage: list");
    return;
  }

  bool any = false;
  int lastDrawOrder = -1;
  while (true) {
    int index = findNextDrawOrderElement(lastDrawOrder);
    if (index < 0) {
      break;
    }
    printTextElement(scene.textElements[index]);
    lastDrawOrder = scene.textElements[index].drawOrder;
    any = true;
  }

  if (!any) {
    Serial.println("Scene is empty");
  }
}

void handleGetCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 2) {
    Serial.println("Usage: get <id>");
    return;
  }

  uint16_t id;
  if (!parseUInt16Value(tokens[1], id)) {
    Serial.println("Invalid id");
    return;
  }

  const TextElement* element = getTextElement(id);
  if (element == nullptr) {
    Serial.print("Text element not found: ");
    Serial.println(id);
    return;
  }

  printTextElement(*element);
}

void handleAddTextCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 7) {
    Serial.println("Usage: ta <x> <y> <size> <r,g,b> <bg> \"text\"");
    return;
  }

  long xValue;
  long yValue;
  long sizeValue;
  int red;
  int green;
  int blue;
  int backgroundValue;

  if (!parseInt32(tokens[1], xValue) || xValue < INT16_MIN || xValue > INT16_MAX) {
    Serial.println("Invalid x. Use a signed 16-bit integer");
    return;
  }
  if (!parseInt32(tokens[2], yValue) || yValue < INT16_MIN || yValue > INT16_MAX) {
    Serial.println("Invalid y. Use a signed 16-bit integer");
    return;
  }
  if (!parseInt32(tokens[3], sizeValue) || sizeValue < 1 || sizeValue > 255) {
    Serial.println("Invalid size. Use an integer from 1 to 255");
    return;
  }
  if (!parseRgb(tokens[4], red, green, blue)) {
    Serial.println("Invalid rgb. Use r,g,b with values from 0 to 255");
    return;
  }
  if (!parseBool01(tokens[5], backgroundValue)) {
    Serial.println("Invalid bg. Use 0 or 1");
    return;
  }

  TextStyle style = defaultTextStyle();
  style.x = static_cast<int16_t>(xValue);
  style.y = static_cast<int16_t>(yValue);
  style.size = static_cast<uint8_t>(sizeValue);
  style.red = static_cast<uint8_t>(red);
  style.green = static_cast<uint8_t>(green);
  style.blue = static_cast<uint8_t>(blue);
  style.backgroundBox = (backgroundValue == 1);

  bool truncated = false;
  uint16_t id = createTextElement(tokens[6], style, &truncated);
  if (id == 0) {
    Serial.println("No free text element slots");
    return;
  }

  Serial.print("Created text element id=");
  Serial.println(id);
  if (truncated) {
    Serial.print("Text truncated to ");
    Serial.print(TEXT_CONTENT_CAPACITY - 1);
    Serial.println(" characters");
  }
  renderScene();
}

void handleCreateTextShortcutCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 2) {
    Serial.println("Usage: tc \"text\"");
    return;
  }

  TextStyle style = defaultTextStyle();
  bool truncated = false;
  uint16_t id = createTextElement(tokens[1], style, &truncated);
  if (id == 0) {
    Serial.println("No free text element slots");
    return;
  }

  Serial.print("Created text element id=");
  Serial.println(id);
  if (truncated) {
    Serial.print("Text truncated to ");
    Serial.print(TEXT_CONTENT_CAPACITY - 1);
    Serial.println(" characters");
  }
  renderScene();
}

void handleDeleteTextCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 2) {
    Serial.println("Usage: td <id>");
    return;
  }

  uint16_t id;
  if (!parseUInt16Value(tokens[1], id)) {
    Serial.println("Invalid id");
    return;
  }

  if (!deleteTextElement(id)) {
    Serial.print("Text element not found: ");
    Serial.println(id);
    return;
  }

  Serial.print("Deleted text element id=");
  Serial.println(id);
  renderScene();
}

void handleTextSetCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 4) {
    Serial.println("Usage: ts <id> <field> <value>");
    return;
  }

  uint16_t id;
  if (!parseUInt16Value(tokens[1], id)) {
    Serial.println("Invalid id");
    return;
  }

  if (!setTextElementField(id, tokens[2], tokens[3])) {
    return;
  }

  Serial.print("Updated text element id=");
  Serial.print(id);
  Serial.print(" field=");
  Serial.println(tokens[2]);
  renderScene();
}

void handleZoneAddCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 5) {
    Serial.println("Usage: za <x> <y> <w> <h>");
    return;
  }

  long xValue;
  long yValue;
  long widthValue;
  long heightValue;
  if (!parseInt32(tokens[1], xValue) || xValue < INT16_MIN || xValue > INT16_MAX) {
    Serial.println("Invalid zone x. Use a signed 16-bit integer");
    return;
  }
  if (!parseInt32(tokens[2], yValue) || yValue < INT16_MIN || yValue > INT16_MAX) {
    Serial.println("Invalid zone y. Use a signed 16-bit integer");
    return;
  }
  if (!parseInt32(tokens[3], widthValue) || widthValue < 1 || widthValue > INT16_MAX) {
    Serial.println("Invalid zone width. Use an integer from 1 to 32767");
    return;
  }
  if (!parseInt32(tokens[4], heightValue) || heightValue < 1 || heightValue > INT16_MAX) {
    Serial.println("Invalid zone height. Use an integer from 1 to 32767");
    return;
  }

  uint16_t id = createZone(static_cast<int16_t>(xValue), static_cast<int16_t>(yValue), static_cast<int16_t>(widthValue), static_cast<int16_t>(heightValue));
  if (id == 0) {
    Serial.println("No free zone slots");
    return;
  }

  Serial.print("Created zone id=");
  Serial.println(id);
  renderScene();
}

void handleZoneDeleteCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 2) {
    Serial.println("Usage: zd <zoneId>");
    return;
  }

  uint16_t id;
  if (!parseUInt16Value(tokens[1], id)) {
    Serial.println("Invalid zone id");
    return;
  }

  if (!deleteZone(id)) {
    if (getZone(id) == nullptr) {
      Serial.print("Zone not found: ");
      Serial.println(id);
    }
    return;
  }

  Serial.print("Deleted zone id=");
  Serial.println(id);
  renderScene();
}

void handleZoneGetCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 2) {
    Serial.println("Usage: zg <zoneId>");
    return;
  }

  uint16_t id;
  if (!parseUInt16Value(tokens[1], id)) {
    Serial.println("Invalid zone id");
    return;
  }

  const Zone* zone = getZone(id);
  if (zone == nullptr) {
    Serial.print("Zone not found: ");
    Serial.println(id);
    return;
  }

  printZone(*zone);
}

void handleZoneListCommand(int tokenCount) {
  if (tokenCount != 1) {
    Serial.println("Usage: zlist");
    return;
  }

  bool any = false;
  for (uint8_t i = 0; i < MAX_ZONES; i++) {
    if (!scene.zones[i].allocated) {
      continue;
    }
    printZone(scene.zones[i]);
    any = true;
  }

  if (!any) {
    Serial.println("No zones defined");
  }
}

void handleZoneSetCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 4) {
    Serial.println("Usage: zs <zoneId> <field> <value>");
    return;
  }

  uint16_t id;
  if (!parseUInt16Value(tokens[1], id)) {
    Serial.println("Invalid zone id");
    return;
  }

  if (!setZoneField(id, tokens[2], tokens[3])) {
    return;
  }

  Serial.print("Updated zone id=");
  Serial.print(id);
  Serial.print(" field=");
  Serial.println(tokens[2]);
  renderScene();
}

void processCommandLine(char* line) {
  char* tokens[8];
  bool unterminatedQuote = false;
  int tokenCount = tokenizeCommand(line, tokens, 8, unterminatedQuote);

  if (unterminatedQuote) {
    Serial.println("Unterminated quote in command");
    return;
  }
  if (tokenCount < 0) {
    Serial.println("Too many tokens in command");
    return;
  }
  if (tokenCount == 0) {
    return;
  }

  if (strcmp(tokens[0], "help") == 0) {
    handleHelpCommand(tokenCount);
  } else if (strcmp(tokens[0], "b") == 0) {
    handleBrightnessCommand(tokenCount, tokens);
  } else if (strcmp(tokens[0], "blink") == 0) {
    handleBlinkCommand(tokenCount, tokens);
  } else if (strcmp(tokens[0], "clear") == 0) {
    handleClearCommand(tokenCount);
  } else if (strcmp(tokens[0], "list") == 0) {
    handleListCommand(tokenCount);
  } else if (strcmp(tokens[0], "get") == 0) {
    handleGetCommand(tokenCount, tokens);
  } else if (strcmp(tokens[0], "ta") == 0) {
    handleAddTextCommand(tokenCount, tokens);
  } else if (strcmp(tokens[0], "tc") == 0) {
    handleCreateTextShortcutCommand(tokenCount, tokens);
  } else if (strcmp(tokens[0], "td") == 0) {
    handleDeleteTextCommand(tokenCount, tokens);
  } else if (strcmp(tokens[0], "ts") == 0) {
    handleTextSetCommand(tokenCount, tokens);
  } else if (strcmp(tokens[0], "za") == 0) {
    handleZoneAddCommand(tokenCount, tokens);
  } else if (strcmp(tokens[0], "zd") == 0) {
    handleZoneDeleteCommand(tokenCount, tokens);
  } else if (strcmp(tokens[0], "zg") == 0) {
    handleZoneGetCommand(tokenCount, tokens);
  } else if (strcmp(tokens[0], "zlist") == 0) {
    handleZoneListCommand(tokenCount);
  } else if (strcmp(tokens[0], "zs") == 0) {
    handleZoneSetCommand(tokenCount, tokens);
  } else {
    Serial.print("Unknown command: ");
    Serial.println(tokens[0]);
    Serial.println("Use help");
  }
}

void pollSerialCommands() {
  while (Serial.available() > 0) {
    char incoming = static_cast<char>(Serial.read());

    if (incoming == '\r') {
      continue;
    }

    if (incoming == '\n') {
      serialLineBuffer[serialLineLength] = '\0';
      processCommandLine(serialLineBuffer);
      serialLineLength = 0;
      serialLineBuffer[0] = '\0';
      continue;
    }

    if (serialLineLength >= SERIAL_LINE_CAPACITY - 1) {
      serialLineLength = 0;
      serialLineBuffer[0] = '\0';
      Serial.println("Command too long");
      while (Serial.available() > 0) {
        char discard = static_cast<char>(Serial.read());
        if (discard == '\n') {
          break;
        }
      }
      return;
    }

    serialLineBuffer[serialLineLength++] = incoming;
  }
}

void createStartupClockScene() {
  TextStyle clockStyle = defaultTextStyle();
  clockStyle.size = 2;

  TextMetrics fullMetrics = measureTextAt("12:34", clockStyle.size, 0, 0);
  TextMetrics hoursMetrics = measureTextAt("12", clockStyle.size, 0, 0);
  TextMetrics colonMetrics = measureTextAt(":", clockStyle.size, 0, 0);

  int16_t groupX = (PANEL_WIDTH - static_cast<int16_t>(fullMetrics.width)) / 2 - fullMetrics.x1;
  int16_t groupY = (PANEL_HEIGHT - static_cast<int16_t>(fullMetrics.height)) / 2 - fullMetrics.y1;

  TextStyle hoursStyle = clockStyle;
  hoursStyle.x = groupX;
  hoursStyle.y = groupY;
  createTextElement("12", hoursStyle, nullptr);

  TextStyle colonStyle = clockStyle;
  colonStyle.x = groupX + static_cast<int16_t>(hoursMetrics.width);
  colonStyle.y = groupY;
  colonStyle.blink = true;
  createTextElement(":", colonStyle, nullptr);

  TextStyle minutesStyle = clockStyle;
  minutesStyle.x = colonStyle.x + static_cast<int16_t>(colonMetrics.width);
  minutesStyle.y = groupY;
  createTextElement("34", minutesStyle, nullptr);

  TextStyle footerStyle = defaultTextStyle();
  footerStyle.size = 1;
  TextMetrics footerMetrics = measureTextAt("x.com/bendesprets", footerStyle.size, 0, 0);
  int16_t footerZoneHeight = static_cast<int16_t>(footerMetrics.height);
  int16_t footerZoneY = PANEL_HEIGHT - footerZoneHeight;
  uint16_t footerZoneId = createZone(0, footerZoneY, PANEL_WIDTH, footerZoneHeight);

  uint16_t footerId = createTextElement("x.com/bendesprets", footerStyle, nullptr);
  TextElement* footer = findTextElementById(footerId);
  if (footer != nullptr) {
    footer->zoneId = footerZoneId;
    footer->style.x = 0;
    footer->style.y = footerZoneHeight - static_cast<int16_t>(footerMetrics.height) - footerMetrics.y1;
    footer->scrollEnabled = true;
    footer->scrollMode = SCROLL_MODE_LOOP;
    footer->scrollSpeedPxPerSec = DEFAULT_SCROLL_SPEED_PX_PER_SEC;
    footer->scrollPauseMs = DEFAULT_SCROLL_PAUSE_MS;
    resetTextAnimation(*footer);
  }
}

void setup() {
  initScene();

  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("clock starting");

  if (!beginMatrix()) {
    Serial.println("Matrix init failed");
    return;
  }

  createStartupClockScene();
  renderScene();

  Serial.println("Matrix init OK");
  Serial.print("Driver mode: ");
  Serial.println(MATRIX_DRIVER_NAME);
  Serial.print("Clock phase: ");
  Serial.println(MATRIX_CLKPHASE ? "true" : "false");
  Serial.print("Latch blanking: ");
  Serial.println(MATRIX_LAT_BLANKING);
  Serial.print("Brightness: ");
  Serial.println(scene.brightness);
  Serial.print("Blink interval: ");
  Serial.print(scene.blinkIntervalMs);
  Serial.println(" ms");
  connectToWifi();
  printHelp();
}

void loop() {
  pollSerialCommands();
  if (updateAnimatedScene(millis())) {
    renderScene();
  }
  delay(5);
}

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ctype.h>
#include <limits.h>
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
#define TEXT_CONTENT_CAPACITY 32
#define SERIAL_LINE_CAPACITY 128

MatrixPanel_I2S_DMA* matrix = nullptr;

struct TextStyle {
  int16_t x;
  int16_t y;
  uint8_t size;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  bool backgroundBox;
  bool visible;
};

struct TextElement {
  bool allocated;
  uint16_t id;
  uint16_t drawOrder;
  TextStyle style;
  char content[TEXT_CONTENT_CAPACITY];
};

struct SceneState {
  uint8_t brightness;
  TextElement textElements[MAX_TEXT_ELEMENTS];
  uint16_t nextTextElementId;
  uint16_t nextDrawOrder;
};

SceneState scene;
char serialLineBuffer[SERIAL_LINE_CAPACITY];
size_t serialLineLength = 0;

bool parseRgb(const char* text, int& red, int& green, int& blue);
bool parseBool01(const char* text, int& value);

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
    true
  );

  config.mx_height = PANEL_HEIGHT;
  config.chain_length = PANELS_NUMBER;
  config.gpio.e = E_PIN;
  config.clkphase = MATRIX_CLKPHASE;

  matrix = new MatrixPanel_I2S_DMA(config);
  matrix->setBrightness8(scene.brightness);
  if (!matrix->begin()) {
    return false;
  }

  matrix->setLatBlanking(MATRIX_LAT_BLANKING);
  return true;
}

void clearScreen() {
  matrix->fillScreenRGB888(0, 0, 0);
}

void presentFrame() {
  matrix->flipDMABuffer();
}

TextStyle defaultTextStyle() {
  TextStyle style = {0, 0, 1, 255, 255, 255, false, true};
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

void resetTextElement(TextElement& element) {
  element.allocated = false;
  element.id = 0;
  element.drawOrder = 0;
  element.style = defaultTextStyle();
  element.content[0] = '\0';
}

void initScene() {
  scene.brightness = PANEL_BRIGHTNESS;
  scene.nextTextElementId = 1;
  scene.nextDrawOrder = 1;

  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
    resetTextElement(scene.textElements[i]);
  }
}

void clearScene() {
  scene.nextTextElementId = 1;
  scene.nextDrawOrder = 1;

  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
    resetTextElement(scene.textElements[i]);
  }
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

uint16_t to565(uint8_t red, uint8_t green, uint8_t blue) {
  return matrix->color565(red, green, blue);
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
  return true;
}

bool setTextElementField(uint16_t id, const char* field, const char* value) {
  TextElement* element = findTextElementById(id);
  if (element == nullptr) {
    Serial.print("Text element not found: ");
    Serial.println(id);
    return false;
  }

  if (strcmp(field, "x") == 0) {
    char* end = nullptr;
    long parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < INT16_MIN || parsed > INT16_MAX) {
      Serial.println("Invalid x. Use a signed 16-bit integer");
      return false;
    }
    element->style.x = static_cast<int16_t>(parsed);
    return true;
  }

  if (strcmp(field, "y") == 0) {
    char* end = nullptr;
    long parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < INT16_MIN || parsed > INT16_MAX) {
      Serial.println("Invalid y. Use a signed 16-bit integer");
      return false;
    }
    element->style.y = static_cast<int16_t>(parsed);
    return true;
  }

  if (strcmp(field, "size") == 0) {
    char* end = nullptr;
    long parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < 1 || parsed > 255) {
      Serial.println("Invalid size. Use an integer from 1 to 255");
      return false;
    }
    element->style.size = static_cast<uint8_t>(parsed);
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
    int parsed;
    if (!parseBool01(value, parsed)) {
      Serial.println("Invalid bg. Use 0 or 1");
      return false;
    }
    element->style.backgroundBox = (parsed == 1);
    return true;
  }

  if (strcmp(field, "vis") == 0) {
    int parsed;
    if (!parseBool01(value, parsed)) {
      Serial.println("Invalid vis. Use 0 or 1");
      return false;
    }
    element->style.visible = (parsed == 1);
    return true;
  }

  Serial.print("Unknown field: ");
  Serial.println(field);
  return false;
}

void drawTextElement(const TextElement& element) {
  if (!element.allocated || !element.style.visible || element.content[0] == '\0') {
    return;
  }

  matrix->setTextWrap(false);
  matrix->setTextSize(element.style.size);

  if (element.style.backgroundBox) {
    int16_t x1;
    int16_t y1;
    uint16_t w;
    uint16_t h;
    matrix->getTextBounds(element.content, element.style.x, element.style.y, &x1, &y1, &w, &h);
    matrix->fillRect(x1, y1, w, h, to565(0, 0, 0));
  }

  matrix->setTextColor(to565(element.style.red, element.style.green, element.style.blue));
  matrix->setCursor(element.style.x, element.style.y);
  matrix->print(element.content);
}

int findNextDrawOrderElement(int lastDrawOrder) {
  int bestIndex = -1;
  int bestDrawOrder = INT_MAX;

  for (uint8_t i = 0; i < MAX_TEXT_ELEMENTS; i++) {
    const TextElement& element = scene.textElements[i];
    if (!element.allocated) {
      continue;
    }

    if (element.drawOrder <= lastDrawOrder) {
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

  int lastDrawOrder = -1;
  while (true) {
    int index = findNextDrawOrderElement(lastDrawOrder);
    if (index < 0) {
      break;
    }

    drawTextElement(scene.textElements[index]);
    lastDrawOrder = scene.textElements[index].drawOrder;
  }

  presentFrame();
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
  Serial.print(" text=\"");
  Serial.print(element.content);
  Serial.println("\"");
}

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  help");
  Serial.println("  b <0-255>");
  Serial.println("  clear");
  Serial.println("  list");
  Serial.println("  get <id>");
  Serial.println("  ta <x> <y> <size> <r,g,b> <bg> \"text\"");
  Serial.println("  td <id>");
  Serial.println("  tc <id> \"text\"");
  Serial.println("  ts <id> <field> <value>");
  Serial.println("Fields: x, y, size, rgb, bg, vis");
  Serial.println("Examples:");
  Serial.println("  b 64");
  Serial.println("  ta 2 12 1 255,255,255 0 \"12:34\"");
  Serial.println("  ts 1 rgb 255,0,0");
  Serial.println("  tc 1 \"HELLO WORLD\"");
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
  if (firstComma == nullptr) {
    return false;
  }

  const char* secondComma = strchr(firstComma + 1, ',');
  if (secondComma == nullptr) {
    return false;
  }

  if (strchr(secondComma + 1, ',') != nullptr) {
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

void handleTextContentCommand(int tokenCount, char* tokens[]) {
  if (tokenCount != 3) {
    Serial.println("Usage: tc <id> \"text\"");
    return;
  }

  uint16_t id;
  if (!parseUInt16Value(tokens[1], id)) {
    Serial.println("Invalid id");
    return;
  }

  bool truncated = false;
  if (!setTextElementContent(id, tokens[2], &truncated)) {
    Serial.print("Text element not found: ");
    Serial.println(id);
    return;
  }

  Serial.print("Updated text for id=");
  Serial.println(id);
  if (truncated) {
    Serial.print("Text truncated to ");
    Serial.print(TEXT_CONTENT_CAPACITY - 1);
    Serial.println(" characters");
  }
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
    return;
  }

  if (strcmp(tokens[0], "b") == 0) {
    handleBrightnessCommand(tokenCount, tokens);
    return;
  }

  if (strcmp(tokens[0], "clear") == 0) {
    handleClearCommand(tokenCount);
    return;
  }

  if (strcmp(tokens[0], "list") == 0) {
    handleListCommand(tokenCount);
    return;
  }

  if (strcmp(tokens[0], "get") == 0) {
    handleGetCommand(tokenCount, tokens);
    return;
  }

  if (strcmp(tokens[0], "ta") == 0) {
    handleAddTextCommand(tokenCount, tokens);
    return;
  }

  if (strcmp(tokens[0], "td") == 0) {
    handleDeleteTextCommand(tokenCount, tokens);
    return;
  }

  if (strcmp(tokens[0], "tc") == 0) {
    handleTextContentCommand(tokenCount, tokens);
    return;
  }

  if (strcmp(tokens[0], "ts") == 0) {
    handleTextSetCommand(tokenCount, tokens);
    return;
  }

  Serial.print("Unknown command: ");
  Serial.println(tokens[0]);
  Serial.println("Use help");
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
  printHelp();
}

void loop() {
  pollSerialCommands();
  delay(5);
}

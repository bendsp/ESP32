#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <FastLED.h>

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

MatrixPanel_I2S_DMA* matrix = nullptr;
uint8_t currentBrightness = PANEL_BRIGHTNESS;
uint16_t timeCounter = 0;
uint16_t plasmaCycles = 0;
uint16_t fps = 0;
unsigned long fpsTimer = 0;
bool plasmaEnabled = false;
uint8_t currentScene = 1;
CRGB currentColor;
CRGBPalette16 palettes[] = {RainbowColors_p, RainbowStripeColors_p, HeatColors_p, CloudColors_p};
CRGBPalette16 currentPalette = palettes[0];
String currentText = "HELLO";
int16_t textX = 2;
int16_t textY = 12;
uint8_t textSize = 1;
uint8_t textRed = 255;
uint8_t textGreen = 255;
uint8_t textBlue = 255;
bool textBackgroundBox = false;
bool autoTextRedraw = true;
bool textSticky = false;

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
  matrix->setBrightness8(currentBrightness);
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

void fillSolid(uint8_t red, uint8_t green, uint8_t blue) {
  matrix->fillScreenRGB888(red, green, blue);
}

void drawPixelSafe(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  if (x < 0 || x >= PANEL_WIDTH || y < 0 || y >= PANEL_HEIGHT) {
    return;
  }

  matrix->drawPixelRGB888(x, y, red, green, blue);
}

void showCornerMarkers() {
  drawPixelSafe(0, 0, 255, 0, 0);
  drawPixelSafe(PANEL_WIDTH - 1, 0, 0, 255, 0);
  drawPixelSafe(0, PANEL_HEIGHT - 1, 0, 0, 255);
  drawPixelSafe(PANEL_WIDTH - 1, PANEL_HEIGHT - 1, 255, 255, 255);
}

void showTestPattern() {
  clearScreen();

  for (int x = 0; x < PANEL_WIDTH; x++) {
    drawPixelSafe(x, PANEL_HEIGHT / 2, 255, 0, 0);
  }

  for (int y = 0; y < PANEL_HEIGHT; y++) {
    drawPixelSafe(PANEL_WIDTH / 2, y, 0, 255, 0);
  }

  showCornerMarkers();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("matrix_playground starting");

  if (!beginMatrix()) {
    Serial.println("Matrix init failed");
    return;
  }

  Serial.println("Matrix init OK");
  Serial.print("Driver mode: ");
  Serial.println(MATRIX_DRIVER_NAME);
  Serial.print("Clock phase: ");
  Serial.println(MATRIX_CLKPHASE ? "true" : "false");
  Serial.print("Latch blanking: ");
  Serial.println(MATRIX_LAT_BLANKING);
  Serial.print("Brightness: ");
  Serial.println(currentBrightness);
  setPlasmaScene(3);
}

void drawHorizontalLineSafe(int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  if (y < 0 || y >= PANEL_HEIGHT) {
    return;
  }

  for (int16_t x = 0; x < PANEL_WIDTH; x++) {
    drawPixelSafe(x, y, red, green, blue);
  }
}

void drawVerticalLineSafe(int16_t x, uint8_t red, uint8_t green, uint8_t blue) {
  if (x < 0 || x >= PANEL_WIDTH) {
    return;
  }

  for (int16_t y = 0; y < PANEL_HEIGHT; y++) {
    drawPixelSafe(x, y, red, green, blue);
  }
}

void colorFromCode(char code, uint8_t& red, uint8_t& green, uint8_t& blue) {
  red = 255;
  green = 255;
  blue = 255;

  switch (code) {
    case 'r':
    case 'R':
      green = 0;
      blue = 0;
      break;
    case 'g':
    case 'G':
      red = 0;
      blue = 0;
      break;
    case 'b':
    case 'B':
      red = 0;
      green = 0;
      break;
    case 'w':
    case 'W':
    default:
      break;
  }
}

void showFull(uint8_t red, uint8_t green, uint8_t blue) {
  clearScreen();
  fillSolid(red, green, blue);
  presentFrame();
  Serial.print("Showing full screen color=");
  Serial.println((red == 255 && green == 0 && blue == 0) ? "r" :
                 (red == 0 && green == 255 && blue == 0) ? "g" :
                 (red == 0 && green == 0 && blue == 255) ? "b" : "w");
}

void setBrightnessLevel(int value) {
  if (value < 0 || value > 255) {
    Serial.println("Brightness out of range. Use b0 to b255");
    return;
  }

  currentBrightness = static_cast<uint8_t>(value);
  matrix->setBrightness8(currentBrightness);
  Serial.print("Brightness set to ");
  Serial.println(currentBrightness);
}

uint16_t to565(uint8_t red, uint8_t green, uint8_t blue) {
  return matrix->color565(red, green, blue);
}

void drawConfiguredTextOverlay() {
  matrix->setTextWrap(false);
  matrix->setTextSize(textSize);

  uint16_t fgColor = to565(textRed, textGreen, textBlue);

  if (textBackgroundBox) {
    int16_t x1;
    int16_t y1;
    uint16_t w;
    uint16_t h;
    matrix->getTextBounds(currentText, textX, textY, &x1, &y1, &w, &h);
    matrix->fillRect(x1, y1, w, h, to565(0, 0, 0));
  }

  matrix->setTextColor(fgColor);
  matrix->setCursor(textX, textY);
  matrix->print(currentText);

  if (!textSticky) {
    Serial.print("Text drawn: \"");
    Serial.print(currentText);
    Serial.print("\" at ");
    Serial.print(textX);
    Serial.print(",");
    Serial.print(textY);
    Serial.print(" size=");
    Serial.print(textSize);
    Serial.print(" fg=");
    Serial.print(textRed);
    Serial.print(",");
    Serial.print(textGreen);
    Serial.print(",");
    Serial.print(textBlue);
    Serial.print(" bgbox=");
    Serial.println(textBackgroundBox ? "on" : "off");
  }
}

void drawConfiguredText() {
  stopPlasma();
  clearScreen();
  drawConfiguredTextOverlay();
  presentFrame();
}

void redrawTextIfEnabled() {
  if (autoTextRedraw) {
    if (textSticky) {
      return;
    }
    drawConfiguredText();
  }
}

void stopPlasma() {
  plasmaEnabled = false;
}

void setPlasmaScene(uint8_t scene) {
  if (scene < 1 || scene > 4) {
    Serial.println("Use rainbow 1, rainbow 2, rainbow 3, or rainbow 4");
    return;
  }

  currentScene = scene;
  currentPalette = palettes[scene - 1];
  timeCounter = 0;
  plasmaCycles = 0;
  fps = 0;
  fpsTimer = millis();
  plasmaEnabled = true;
  clearScreen();

  Serial.print("Plasma scene enabled: ");
  Serial.println(scene);
}

void renderPlasmaFrame() {
  for (int x = 0; x < PANEL_WIDTH; x++) {
    for (int y = 0; y < PANEL_HEIGHT; y++) {
      int16_t value = 128;
      uint8_t wobble = sin8(timeCounter);

      value += sin16(x * wobble * 3 + timeCounter);
      value += cos16(y * (128 - wobble) + timeCounter);
      value += sin16(y * x * cos8(-timeCounter) / 8);

      currentColor = ColorFromPalette(currentPalette, (value >> 8));
      drawPixelSafe(x, y, currentColor.r, currentColor.g, currentColor.b);
    }
  }

  ++timeCounter;
  ++plasmaCycles;
  ++fps;

  if (plasmaCycles >= 1024) {
    timeCounter = 0;
    plasmaCycles = 0;
  }

  if (fpsTimer + 5000 < millis()) {
    Serial.printf("Plasma fps: %d\n", fps / 5);
    fpsTimer = millis();
    fps = 0;
  }

  if (textSticky) {
    drawConfiguredTextOverlay();
  }

  presentFrame();
}

void showRow(int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  stopPlasma();
  clearScreen();
  drawHorizontalLineSafe(y, red, green, blue);
  presentFrame();
  Serial.print("Showing logical row y=");
  Serial.print(y);
  Serial.print(" color=");
  Serial.println((red == 255 && green == 0 && blue == 0) ? "r" :
                 (red == 0 && green == 255 && blue == 0) ? "g" :
                 (red == 0 && green == 0 && blue == 255) ? "b" : "w");
}

void showColumn(int16_t x, uint8_t red, uint8_t green, uint8_t blue) {
  stopPlasma();
  clearScreen();
  drawVerticalLineSafe(x, red, green, blue);
  presentFrame();
  Serial.print("Showing logical column x=");
  Serial.print(x);
  Serial.print(" color=");
  Serial.println((red == 255 && green == 0 && blue == 0) ? "r" :
                 (red == 0 && green == 255 && blue == 0) ? "g" :
                 (red == 0 && green == 0 && blue == 255) ? "b" : "w");
}

void showPixel(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  if (x < 0 || x >= PANEL_WIDTH || y < 0 || y >= PANEL_HEIGHT) {
    Serial.println("Pixel out of range. Use x and y from 0 to 63");
    return;
  }

  stopPlasma();
  clearScreen();
  drawPixelSafe(x, y, red, green, blue);
  presentFrame();
  Serial.print("Showing logical pixel x=");
  Serial.print(x);
  Serial.print(" y=");
  Serial.print(y);
  Serial.print(" color=");
  Serial.println((red == 255 && green == 0 && blue == 0) ? "r" :
                 (red == 0 && green == 255 && blue == 0) ? "g" :
                 (red == 0 && green == 0 && blue == 255) ? "b" : "w");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil(10);
    command.trim();

    if (command.length() < 2) {
      if (command == "r") {
        command = "rainbow 1";
      } else {
        Serial.println("Use x/y/p/full/b/rainbow commands, or txt/tx/ty/ts/tfg/tbg/drawtext");
        return;
      }
    }

    if (command == "rainbow 1" || command == "rainbow 2" || command == "rainbow 3" || command == "rainbow 4") {
      setPlasmaScene(command.charAt(command.length() - 1) - '0');
      return;
    }

    if (command.startsWith("txt ")) {
      currentText = command.substring(4);
      Serial.print("Text set to: ");
      Serial.println(currentText);
      redrawTextIfEnabled();
      return;
    }

    if (command.startsWith("tx ")) {
      textX = command.substring(3).toInt();
      Serial.print("Text X set to ");
      Serial.println(textX);
      redrawTextIfEnabled();
      return;
    }

    if (command.startsWith("ty ")) {
      textY = command.substring(3).toInt();
      Serial.print("Text Y set to ");
      Serial.println(textY);
      redrawTextIfEnabled();
      return;
    }

    if (command.startsWith("ts ")) {
      int value = command.substring(3).toInt();
      if (value < 1) {
        Serial.println("Text size must be >= 1");
      } else {
        textSize = static_cast<uint8_t>(value);
        Serial.print("Text size set to ");
        Serial.println(textSize);
        redrawTextIfEnabled();
      }
      return;
    }

    if (command.startsWith("tfg ")) {
      String rgb = command.substring(4);
      int firstComma = rgb.indexOf(',');
      int secondComma = rgb.indexOf(',', firstComma + 1);
      if (firstComma < 0 || secondComma < 0) {
        Serial.println("Use tfg r,g,b for example tfg 255,255,255");
        return;
      }

      textRed = static_cast<uint8_t>(rgb.substring(0, firstComma).toInt());
      textGreen = static_cast<uint8_t>(rgb.substring(firstComma + 1, secondComma).toInt());
      textBlue = static_cast<uint8_t>(rgb.substring(secondComma + 1).toInt());
      Serial.print("Text color set to ");
      Serial.print(textRed);
      Serial.print(",");
      Serial.print(textGreen);
      Serial.print(",");
      Serial.println(textBlue);
      redrawTextIfEnabled();
      return;
    }

    if (command == "tbg on") {
      textBackgroundBox = true;
      Serial.println("Text background box enabled");
      redrawTextIfEnabled();
      return;
    }

    if (command == "tbg off") {
      textBackgroundBox = false;
      Serial.println("Text background box disabled");
      redrawTextIfEnabled();
      return;
    }

    if (command == "tsticky on") {
      textSticky = true;
      Serial.println("Sticky text overlay enabled");
      if (!plasmaEnabled) {
        drawConfiguredText();
      }
      return;
    }

    if (command == "tsticky off") {
      textSticky = false;
      Serial.println("Sticky text overlay disabled");
      return;
    }

    if (command == "drawtext") {
      drawConfiguredText();
      return;
    }

    char axis = command.charAt(0);
    char colorCode = command.charAt(command.length() - 1);
    bool hasColorSuffix = (colorCode == 'r' || colorCode == 'R' || colorCode == 'g' || colorCode == 'G' || colorCode == 'b' || colorCode == 'B' || colorCode == 'w' || colorCode == 'W');

    if (axis == 'b' || axis == 'B') {
      int value = command.substring(1).toInt();
      setBrightnessLevel(value);
      return;
    }

    uint8_t red;
    uint8_t green;
    uint8_t blue;
    colorFromCode(hasColorSuffix ? colorCode : 'w', red, green, blue);

    if (command.startsWith("full")) {
      stopPlasma();
      showFull(red, green, blue);
      return;
    }

    if (axis == 'p' || axis == 'P') {
      String coords = hasColorSuffix ? command.substring(1, command.length() - 1) : command.substring(1);
      int commaIndex = coords.indexOf(',');
      if (commaIndex < 0) {
        Serial.println("Use p<x>,<y><color>, for example p63,0w or p12,34r");
        return;
      }

      int x = coords.substring(0, commaIndex).toInt();
      int y = coords.substring(commaIndex + 1).toInt();
      showPixel(x, y, red, green, blue);
      return;
    }

    String numberPart = hasColorSuffix ? command.substring(1, command.length() - 1) : command.substring(1);
    int value = numberPart.toInt();

    if (axis == 'x' || axis == 'X') {
      if (value >= 0 && value < PANEL_WIDTH) {
        showColumn(value, red, green, blue);
      } else {
        Serial.println("Column out of range. Use x0 to x63");
      }
    } else if (axis == 'y' || axis == 'Y') {
      if (value >= 0 && value < PANEL_HEIGHT) {
        showRow(value, red, green, blue);
      } else {
        Serial.println("Row out of range. Use y0 to y63");
      }
    } else {
      Serial.println("Use x/y/p/full/b/rainbow commands, or txt/tx/ty/ts/tfg/tbg/drawtext");
    }
  }

  if (plasmaEnabled) {
    renderPlasmaFrame();
  }
}

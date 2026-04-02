#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64
#define PANELS_NUMBER 1
#define PANEL_BRIGHTNESS 128

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
    HUB75_I2S_CFG::SHIFTREG
  );

  config.mx_height = PANEL_HEIGHT;
  config.chain_length = PANELS_NUMBER;
  config.gpio.e = E_PIN;

  matrix = new MatrixPanel_I2S_DMA(config);
  matrix->setBrightness8(PANEL_BRIGHTNESS);
  return matrix->begin();
}

void clearScreen() {
  matrix->fillScreenRGB888(0, 0, 0);
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

int16_t mapWrappedX(int16_t x) {
  if (x < 0 || x >= PANEL_WIDTH) {
    return x;
  }

  return (x + 1) % PANEL_WIDTH;
}

void drawPixelMapped(int16_t x, int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  drawPixelSafe(mapWrappedX(x), y, red, green, blue);
}

void showCornerMarkers() {
  drawPixelMapped(0, 0, 255, 0, 0);
  drawPixelMapped(PANEL_WIDTH - 1, 0, 0, 255, 0);
  drawPixelMapped(0, PANEL_HEIGHT - 1, 0, 0, 255);
  drawPixelMapped(PANEL_WIDTH - 1, PANEL_HEIGHT - 1, 255, 255, 255);
}

void showTestPattern() {
  clearScreen();

  for (int x = 0; x < PANEL_WIDTH; x++) {
    drawPixelMapped(x, PANEL_HEIGHT / 2, 255, 0, 0);
  }

  for (int y = 0; y < PANEL_HEIGHT; y++) {
    drawPixelMapped(PANEL_WIDTH / 2, y, 0, 255, 0);
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
}

void drawHorizontalLineSafe(int16_t y, uint8_t red, uint8_t green, uint8_t blue) {
  if (y < 0 || y >= PANEL_HEIGHT) {
    return;
  }

  for (int16_t x = 0; x < PANEL_WIDTH; x++) {
    drawPixelMapped(x, y, red, green, blue);
  }
}

void drawVerticalLineSafe(int16_t x, uint8_t red, uint8_t green, uint8_t blue) {
  if (x < 0 || x >= PANEL_WIDTH) {
    return;
  }

  for (int16_t y = 0; y < PANEL_HEIGHT; y++) {
    drawPixelMapped(x, y, red, green, blue);
  }
}

void showRow(int16_t y) {
  clearScreen();
  drawHorizontalLineSafe(y, 255, 255, 255);
  Serial.print("Showing logical row y=");
  Serial.println(y);
}

void showColumn(int16_t x) {
  clearScreen();
  drawVerticalLineSafe(x, 255, 255, 255);
  Serial.print("Showing logical column x=");
  Serial.println(x);
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil(10);
    command.trim();

    if (command.length() < 2) {
      Serial.println("Use x<number> or y<number>, for example x5 or y12");
      return;
    }

    char axis = command.charAt(0);
    int value = command.substring(1).toInt();

    if (axis == 'x' || axis == 'X') {
      if (value >= 0 && value < PANEL_WIDTH) {
        showColumn(value);
      } else {
        Serial.println("Column out of range. Use x0 to x63");
      }
    } else if (axis == 'y' || axis == 'Y') {
      if (value >= 0 && value < PANEL_HEIGHT) {
        showRow(value);
      } else {
        Serial.println("Row out of range. Use y0 to y63");
      }
    } else {
      Serial.println("Use x<number> or y<number>, for example x5 or y12");
    }
  }
}

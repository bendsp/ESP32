#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

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

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("clock starting");

  if (!beginMatrix()) {
    Serial.println("Matrix init failed");
    return;
  }

  clearScreen();
  presentFrame();
}

void loop() {
  delay(100);
}

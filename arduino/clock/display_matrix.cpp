#include "display_matrix.h"

namespace {

constexpr int8_t kR1Pin = 37;
constexpr int8_t kG1Pin = 6;
constexpr int8_t kB1Pin = 36;
constexpr int8_t kR2Pin = 35;
constexpr int8_t kG2Pin = 5;
constexpr int8_t kB2Pin = 0;
constexpr int8_t kAPin = 45;
constexpr int8_t kBPin = 1;
constexpr int8_t kCPin = 48;
constexpr int8_t kDPin = 2;
constexpr int8_t kEPin = 4;
constexpr int8_t kClkPin = 47;
constexpr int8_t kLatPin = 38;
constexpr int8_t kOePin = 21;

}  // namespace

DisplayMatrix::DisplayMatrix(const HUB75_I2S_CFG& config)
    : MatrixPanel_I2S_DMA(config), clipEnabled(false), clipX(0), clipY(0), clipW(0), clipH(0) {}

void DisplayMatrix::setClipRect(int16_t x, int16_t y, int16_t w, int16_t h) {
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

void DisplayMatrix::clearClipRect() {
  clipEnabled = false;
}

void DisplayMatrix::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (!isPointVisible(x, y)) {
    return;
  }

  MatrixPanel_I2S_DMA::drawPixel(x, y, color);
}

void DisplayMatrix::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  if (!clipLineHorizontal(x, y, w)) {
    return;
  }

  MatrixPanel_I2S_DMA::drawFastHLine(x, y, w, color);
}

void DisplayMatrix::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if (!clipLineVertical(x, y, h)) {
    return;
  }

  MatrixPanel_I2S_DMA::drawFastVLine(x, y, h, color);
}

void DisplayMatrix::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if (!clipRect(x, y, w, h)) {
    return;
  }

  MatrixPanel_I2S_DMA::fillRect(x, y, w, h, color);
}

bool DisplayMatrix::isPointVisible(int16_t x, int16_t y) const {
  if (!clipEnabled) {
    return true;
  }

  return x >= clipX && x < clipX + clipW && y >= clipY && y < clipY + clipH;
}

bool DisplayMatrix::clipLineHorizontal(int16_t& x, int16_t y, int16_t& w) const {
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

bool DisplayMatrix::clipLineVertical(int16_t x, int16_t& y, int16_t& h) const {
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

bool DisplayMatrix::clipRect(int16_t& x, int16_t& y, int16_t& w, int16_t& h) const {
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

void initDisplayContext(DisplayContext& context) {
  context.matrix = nullptr;
  context.brightness = kDisplayMatrixDefaultBrightness;
}

bool beginMatrixDisplay(DisplayContext& context) {
  if (context.matrix != nullptr) {
    return true;
  }

  HUB75_I2S_CFG::i2s_pins pins = {
    kR1Pin, kG1Pin, kB1Pin,
    kR2Pin, kG2Pin, kB2Pin,
    kAPin, kBPin, kCPin, kDPin, kEPin,
    kLatPin, kOePin, kClkPin
  };

  HUB75_I2S_CFG config(
      kDisplayMatrixPanelWidth,
      kDisplayMatrixPanelHeight,
      kDisplayMatrixPanelsNumber,
      pins,
      HUB75_I2S_CFG::FM6126A,
      HUB75_I2S_CFG::TYPE138,
      true);

  config.mx_height = kDisplayMatrixPanelHeight;
  config.chain_length = kDisplayMatrixPanelsNumber;
  config.gpio.e = kEPin;
  config.clkphase = kDisplayMatrixClockPhase;

  context.matrix = new DisplayMatrix(config);
  context.matrix->setBrightness8(context.brightness);
  if (!context.matrix->begin()) {
    delete context.matrix;
    context.matrix = nullptr;
    return false;
  }

  context.matrix->setLatBlanking(kDisplayMatrixLatBlanking);
  return true;
}

DisplayMatrix* clippedMatrix(DisplayContext& context) {
  return context.matrix;
}

const DisplayMatrix* clippedMatrix(const DisplayContext& context) {
  return context.matrix;
}

MatrixPanel_I2S_DMA* rawMatrix(DisplayContext& context) {
  return context.matrix;
}

const MatrixPanel_I2S_DMA* rawMatrix(const DisplayContext& context) {
  return context.matrix;
}

void setDisplayBrightness(DisplayContext& context, uint8_t brightness) {
  context.brightness = brightness;
  if (context.matrix != nullptr) {
    context.matrix->setBrightness8(brightness);
  }
}

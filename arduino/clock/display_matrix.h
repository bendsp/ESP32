#ifndef DISPLAY_MATRIX_H
#define DISPLAY_MATRIX_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <stdint.h>

inline constexpr int16_t kDisplayMatrixPanelWidth = 64;
inline constexpr int16_t kDisplayMatrixPanelHeight = 64;
inline constexpr uint8_t kDisplayMatrixPanelsNumber = 1;
inline constexpr uint8_t kDisplayMatrixDefaultBrightness = 32;
inline constexpr const char kDisplayMatrixDriverName[] = "FM6126A";
inline constexpr bool kDisplayMatrixClockPhase = false;
inline constexpr uint8_t kDisplayMatrixLatBlanking = 1;

class DisplayMatrix : public MatrixPanel_I2S_DMA {
public:
  explicit DisplayMatrix(const HUB75_I2S_CFG& config);

  void setClipRect(int16_t x, int16_t y, int16_t w, int16_t h);
  void clearClipRect();

  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override;
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override;
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;

private:
  bool clipEnabled;
  int16_t clipX;
  int16_t clipY;
  int16_t clipW;
  int16_t clipH;

  bool isPointVisible(int16_t x, int16_t y) const;
  bool clipLineHorizontal(int16_t& x, int16_t y, int16_t& w) const;
  bool clipLineVertical(int16_t x, int16_t& y, int16_t& h) const;
  bool clipRect(int16_t& x, int16_t& y, int16_t& w, int16_t& h) const;
};

struct DisplayContext {
  DisplayMatrix* matrix;
  uint8_t brightness;
};

void initDisplayContext(DisplayContext& context);
bool beginMatrixDisplay(DisplayContext& context);
DisplayMatrix* clippedMatrix(DisplayContext& context);
const DisplayMatrix* clippedMatrix(const DisplayContext& context);
MatrixPanel_I2S_DMA* rawMatrix(DisplayContext& context);
const MatrixPanel_I2S_DMA* rawMatrix(const DisplayContext& context);
void setDisplayBrightness(DisplayContext& context, uint8_t brightness);

#endif

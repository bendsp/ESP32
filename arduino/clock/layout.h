#ifndef LAYOUT_H
#define LAYOUT_H

#include <stdint.h>

struct LayoutRect {
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
};

struct LayoutInsets {
  int16_t left;
  int16_t top;
  int16_t right;
  int16_t bottom;
};

struct WidgetMetrics {
  int16_t preferredWidth;
  int16_t preferredHeight;
  int16_t minWidth;
  int16_t minHeight;
};

struct WidgetPlacement {
  LayoutRect slotRect;
  LayoutRect contentRect;
};

inline constexpr int16_t kLayoutColumnWidth = 8;
inline constexpr int16_t kLayoutCanvasWidth = 64;
inline constexpr int16_t kLayoutCanvasHeight = 64;
inline constexpr int16_t kLayoutTopRowHeight = 32;

LayoutRect makeLayoutRect(int16_t x, int16_t y, int16_t width, int16_t height);
LayoutRect insetLayoutRect(const LayoutRect& rect, const LayoutInsets& insets);
LayoutRect makeColumnSlotRect(uint8_t columnStart, uint8_t columnSpan, int16_t y, int16_t height);
WidgetPlacement makeWidgetPlacement(const LayoutRect& slotRect, const LayoutInsets& insets);

#endif

#include "layout.h"

LayoutRect makeLayoutRect(int16_t x, int16_t y, int16_t width, int16_t height) {
  LayoutRect rect = {x, y, width, height};
  return rect;
}

LayoutRect insetLayoutRect(const LayoutRect& rect, const LayoutInsets& insets) {
  int16_t insetWidth = rect.width - insets.left - insets.right;
  int16_t insetHeight = rect.height - insets.top - insets.bottom;
  if (insetWidth < 0) {
    insetWidth = 0;
  }
  if (insetHeight < 0) {
    insetHeight = 0;
  }

  return makeLayoutRect(rect.x + insets.left, rect.y + insets.top, insetWidth, insetHeight);
}

LayoutRect makeColumnSlotRect(uint8_t columnStart, uint8_t columnSpan, int16_t y, int16_t height) {
  return makeLayoutRect(
      static_cast<int16_t>(columnStart) * kLayoutColumnWidth,
      y,
      static_cast<int16_t>(columnSpan) * kLayoutColumnWidth,
      height);
}

WidgetPlacement makeWidgetPlacement(const LayoutRect& slotRect, const LayoutInsets& insets) {
  WidgetPlacement placement = {slotRect, insetLayoutRect(slotRect, insets)};
  return placement;
}

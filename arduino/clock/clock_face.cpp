#include "clock_face.h"

#include "display_matrix.h"

namespace {

constexpr LayoutInsets kTopWidgetInsets = {0, 0, 0, 0};
constexpr int16_t kTopSlotColumnSpan = 4;
constexpr int16_t kClockBlockTopY = 11;

WidgetPlacement topSlotPlacement(bool rightSide) {
  uint8_t columnStart = rightSide ? kTopSlotColumnSpan : 0;
  LayoutRect slotRect = makeColumnSlotRect(columnStart, kTopSlotColumnSpan, 0, kLayoutTopRowHeight);
  return makeWidgetPlacement(slotRect, kTopWidgetInsets);
}

void placeTopWidget(
    ClockTopWidget widgetKind,
    ClockFaceState& state,
    MatrixPanel_I2S_DMA* matrix,
    const WidgetPlacement& placement) {
  if (widgetKind == CLOCK_TOP_WIDGET_WEATHER) {
    placeWeatherWidget(state.weatherWidget, state.scene, matrix, placement);
    return;
  }

  placeDateWidget(state.dateWidget, state.scene, matrix, placement);
}

void placeTimeBlock(ClockFaceState& state, MatrixPanel_I2S_DMA* matrix) {
  if (matrix == nullptr || state.hoursTextId == 0 || state.colonTextId == 0 || state.minutesTextId == 0) {
    return;
  }

  TextStyle clockStyle = defaultTextStyle();
  clockStyle.size = 2;

  TextMetrics fullMetrics = measureTextAt(matrix, "XX:XX", clockStyle.size, 0, 0);
  TextMetrics hoursMetrics = measureTextAt(matrix, "XX", clockStyle.size, 0, 0);
  TextMetrics colonMetrics = measureTextAt(matrix, ":", clockStyle.size, 0, 0);

  int16_t groupX = (kDisplayMatrixPanelWidth - static_cast<int16_t>(fullMetrics.width)) / 2 - fullMetrics.x1;
  int16_t groupY = kClockBlockTopY - fullMetrics.y1;
  LayoutRect timeRect = makeLayoutRect(
      groupX + fullMetrics.x1,
      groupY + fullMetrics.y1,
      static_cast<int16_t>(fullMetrics.width),
      static_cast<int16_t>(fullMetrics.height));

  setTextElementPosition(state.scene, state.hoursTextId, groupX, groupY);
  setTextElementPosition(
      state.scene,
      state.colonTextId,
      groupX + static_cast<int16_t>(hoursMetrics.width),
      groupY);
  setTextElementPosition(
      state.scene,
      state.minutesTextId,
      groupX + static_cast<int16_t>(hoursMetrics.width) + static_cast<int16_t>(colonMetrics.width),
      groupY);

  if (state.overlay.debugRectCount >= 3) {
    state.overlay.debugRectCount = 2;
  }
  state.overlay.debugRects[2] = timeRect;
  if (state.overlay.debugRectCount < 3) {
    state.overlay.debugRectCount = 3;
  }
}

bool updateClockDisplayFromRtc(ClockFaceState& state, TimeServiceState& timeService) {
  if (state.hoursTextId == 0 || state.minutesTextId == 0) {
    return false;
  }

  tm timeInfo;
  if (!readCurrentLocalTime(timeService, timeInfo, 10)) {
    return false;
  }

  char hoursText[3];
  char minutesText[3];
  snprintf(hoursText, sizeof(hoursText), "%02d", timeInfo.tm_hour);
  snprintf(minutesText, sizeof(minutesText), "%02d", timeInfo.tm_min);

  bool changed = false;
  const TextElement* hoursElement = getTextElement(state.scene, state.hoursTextId);
  if (hoursElement != nullptr && strcmp(hoursElement->content, hoursText) != 0) {
    setTextElementContent(state.scene, state.hoursTextId, hoursText, nullptr);
    changed = true;
  }

  const TextElement* minutesElement = getTextElement(state.scene, state.minutesTextId);
  if (minutesElement != nullptr && strcmp(minutesElement->content, minutesText) != 0) {
    setTextElementContent(state.scene, state.minutesTextId, minutesText, nullptr);
    changed = true;
  }

  return changed;
}

}  // namespace

ClockFaceArrangement defaultClockFaceArrangement() {
  ClockFaceArrangement arrangement = {CLOCK_TOP_WIDGET_DATE, CLOCK_TOP_WIDGET_WEATHER};
  return arrangement;
}

void initClockFace(ClockFaceState& state, MatrixPanel_I2S_DMA* matrix) {
  initScene(state.scene);
  clearSceneRenderOverlay(state.overlay);
  state.arrangement = defaultClockFaceArrangement();
  state.dateWidget = {};
  state.weatherWidget = {};
  state.hoursTextId = 0;
  state.colonTextId = 0;
  state.minutesTextId = 0;
  state.dirty = true;

  if (matrix == nullptr) {
    return;
  }

  initDateWidget(state.dateWidget, state.scene);
  initWeatherWidget(state.weatherWidget, state.scene);

  TextStyle clockStyle = defaultTextStyle();
  clockStyle.size = 2;

  TextStyle hoursStyle = clockStyle;
  state.hoursTextId = createTextElement(state.scene, "XX", hoursStyle, nullptr);

  TextStyle colonStyle = clockStyle;
  colonStyle.blink = true;
  state.colonTextId = createTextElement(state.scene, ":", colonStyle, nullptr);

  TextStyle minutesStyle = clockStyle;
  state.minutesTextId = createTextElement(state.scene, "XX", minutesStyle, nullptr);

  applyClockFaceArrangement(state, matrix, state.arrangement);
}

void applyClockFaceArrangement(ClockFaceState& state, MatrixPanel_I2S_DMA* matrix, const ClockFaceArrangement& arrangement) {
  state.arrangement = arrangement;

  WidgetPlacement leftPlacement = topSlotPlacement(false);
  WidgetPlacement rightPlacement = topSlotPlacement(true);
  placeTopWidget(state.arrangement.leftTopWidget, state, matrix, leftPlacement);
  placeTopWidget(state.arrangement.rightTopWidget, state, matrix, rightPlacement);
  state.overlay.debugCornersVisible = true;
  state.overlay.debugRectCount = 2;
  state.overlay.debugRects[0] = leftPlacement.slotRect;
  state.overlay.debugRects[1] = rightPlacement.slotRect;
  state.overlay.weatherIconVisible = state.weatherWidget.iconVisible;
  state.overlay.weatherIconKind = state.weatherWidget.iconKind;
  state.overlay.weatherIconX = state.weatherWidget.iconX;
  state.overlay.weatherIconY = state.weatherWidget.iconY;
  placeTimeBlock(state, matrix);
  state.dirty = true;
}

void tickClockFace(
    ClockFaceState& state,
    MatrixPanel_I2S_DMA* matrix,
    TimeServiceState& timeService,
    const WeatherServiceState& weatherService) {
  bool changed = false;
  changed = updateDateWidget(state.dateWidget, state.scene, timeService) || changed;
  changed = updateWeatherWidget(state.weatherWidget, state.scene, matrix, weatherService, state.overlay) || changed;
  changed = updateClockDisplayFromRtc(state, timeService) || changed;
  state.dirty = state.dirty || changed;
}

bool clockFaceIsDirty(const ClockFaceState& state) {
  return state.dirty;
}

void clearClockFaceDirty(ClockFaceState& state) {
  state.dirty = false;
}

SceneState& clockFaceScene(ClockFaceState& state) {
  return state.scene;
}

const SceneState& clockFaceScene(const ClockFaceState& state) {
  return state.scene;
}

const SceneRenderOverlay* clockFaceOverlay(const ClockFaceState& state) {
  return &state.overlay;
}

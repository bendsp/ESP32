#include "clock_face.h"
#include "display_matrix.h"
#include "scene_anim.h"
#include "scene_render.h"
#include "time_service.h"
#include "weather_service.h"

#include <esp_system.h>

namespace {

DisplayContext display;
TimeServiceState timeService;
WeatherServiceState weatherService;
ClockFaceState clockFace;

const char* getResetReasonLabel(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_UNKNOWN:
      return "unknown";
    case ESP_RST_POWERON:
      return "power-on";
    case ESP_RST_EXT:
      return "external pin";
    case ESP_RST_SW:
      return "software";
    case ESP_RST_PANIC:
      return "panic";
    case ESP_RST_INT_WDT:
      return "interrupt watchdog";
    case ESP_RST_TASK_WDT:
      return "task watchdog";
    case ESP_RST_WDT:
      return "other watchdog";
    case ESP_RST_DEEPSLEEP:
      return "deep sleep";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_SDIO:
      return "sdio";
    case ESP_RST_USB:
      return "usb";
    case ESP_RST_JTAG:
      return "jtag";
    case ESP_RST_EFUSE:
      return "efuse";
    case ESP_RST_PWR_GLITCH:
      return "power glitch";
    case ESP_RST_CPU_LOCKUP:
      return "cpu lockup";
    default:
      return "unmapped";
  }
}

void printResetReason() {
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.print("Reset reason: ");
  Serial.print(getResetReasonLabel(reason));
  Serial.print(" (");
  Serial.print(static_cast<int>(reason));
  Serial.println(")");
}

}  // namespace

void setup() {
  initDisplayContext(display);
  initTimeServiceState(timeService);
  initWeatherServiceState(weatherService);

  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("clock starting");
  printResetReason();

  if (!beginMatrixDisplay(display)) {
    Serial.println("Matrix init failed");
    return;
  }

  initClockFace(clockFace, rawMatrix(display));
  renderScene(display, clockFaceScene(clockFace), clockFaceOverlay(clockFace));
  clearClockFaceDirty(clockFace);

  Serial.println("Matrix init OK");
  Serial.print("Driver mode: ");
  Serial.println(kDisplayMatrixDriverName);
  Serial.print("Clock phase: ");
  Serial.println(kDisplayMatrixClockPhase ? "true" : "false");
  Serial.print("Latch blanking: ");
  Serial.println(kDisplayMatrixLatBlanking);
  Serial.print("Brightness: ");
  Serial.println(display.brightness);
  Serial.print("Blink interval: ");
  Serial.print(clockFaceScene(clockFace).blinkIntervalMs);
  Serial.println(" ms");
}

void loop() {
  unsigned long nowMs = millis();

  tickTimeService(timeService);
  tickWeatherService(weatherService, isTimeServiceWifiConnected(timeService), nowMs);
  tickClockFace(clockFace, timeService, weatherService);

  bool dirty = clockFaceIsDirty(clockFace);
  dirty = updateSceneAnimations(clockFaceScene(clockFace), rawMatrix(display), nowMs) || dirty;
  if (dirty) {
    renderScene(display, clockFaceScene(clockFace), clockFaceOverlay(clockFace));
    clearClockFaceDirty(clockFace);
  }

  delay(5);
}

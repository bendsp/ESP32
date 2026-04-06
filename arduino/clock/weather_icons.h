#ifndef WEATHER_ICONS_H
#define WEATHER_ICONS_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <stdint.h>

enum WeatherIconKind : uint8_t {
  WEATHER_ICON_NONE = 0,
  WEATHER_ICON_CLEAR_DAY,
  WEATHER_ICON_CLEAR_NIGHT,
  WEATHER_ICON_PARTLY_CLOUDY_DAY,
  WEATHER_ICON_PARTLY_CLOUDY_NIGHT,
  WEATHER_ICON_CLOUDY,
  WEATHER_ICON_FOG,
  WEATHER_ICON_RAIN,
  WEATHER_ICON_SNOW,
  WEATHER_ICON_STORM
};

namespace weather_icons {

// Icons taken from https://github.com/caternuson/rpi-weather/blob/master/led8x8icons.py
static constexpr uint64_t SUNNY = 0x9142183dbc184289ULL;
static constexpr uint64_t RAIN = 0x55aa55aa55aa55aaULL;
static constexpr uint64_t CLOUD = 0x00007e818999710eULL;
static constexpr uint64_t SHOWERS = 0x152a7e818191710eULL;
static constexpr uint64_t SNOW = 0xa542a51818a542a5ULL;
static constexpr uint64_t STORM = 0x0a04087e8191710eULL;
static constexpr uint64_t UNKNOWN = 0x00004438006c6c00ULL;

inline WeatherIconKind iconKindForWeatherCode(uint8_t weatherCode, bool isDay) {
  switch (weatherCode) {
    case 0:
      return isDay ? WEATHER_ICON_CLEAR_DAY : WEATHER_ICON_CLEAR_NIGHT;
    case 1:
    case 2:
      return isDay ? WEATHER_ICON_PARTLY_CLOUDY_DAY : WEATHER_ICON_PARTLY_CLOUDY_NIGHT;
    case 3:
      return WEATHER_ICON_CLOUDY;
    case 45:
    case 48:
      return WEATHER_ICON_FOG;
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
    case 61:
    case 63:
    case 65:
    case 66:
    case 67:
    case 80:
    case 81:
    case 82:
      return WEATHER_ICON_RAIN;
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:
      return WEATHER_ICON_SNOW;
    case 95:
    case 96:
    case 99:
      return WEATHER_ICON_STORM;
    default:
      return WEATHER_ICON_CLOUDY;
  }
}

inline void drawWeatherValue64(MatrixPanel_I2S_DMA* matrix, uint64_t value, int16_t x, int16_t y, uint16_t color) {
  for (uint8_t row = 0; row < 8; row++) {
    uint8_t rowBits = static_cast<uint8_t>((value >> (row * 8)) & 0xFFU);
    for (uint8_t column = 0; column < 8; column++) {
      if (((rowBits >> column) & 0x01U) == 0) {
        continue;
      }

      matrix->drawPixel(x + column, y + row, color);
    }
  }
}

inline void drawWeatherIcon(MatrixPanel_I2S_DMA* matrix, WeatherIconKind kind, int16_t x, int16_t y) {
  uint64_t icon = 0;
  uint16_t color = matrix->color565(255, 255, 255);

  switch (kind) {
    case WEATHER_ICON_CLEAR_DAY:
      icon = SUNNY;
      color = matrix->color565(255, 220, 0);
      break;
    case WEATHER_ICON_CLEAR_NIGHT:
      icon = UNKNOWN;
      color = matrix->color565(120, 170, 255);
      break;
    case WEATHER_ICON_PARTLY_CLOUDY_DAY:
      icon = CLOUD;
      color = matrix->color565(220, 220, 220);
      break;
    case WEATHER_ICON_PARTLY_CLOUDY_NIGHT:
      icon = CLOUD;
      color = matrix->color565(170, 170, 200);
      break;
    case WEATHER_ICON_CLOUDY:
      icon = CLOUD;
      color = matrix->color565(220, 220, 220);
      break;
    case WEATHER_ICON_FOG:
      icon = UNKNOWN;
      color = matrix->color565(170, 170, 170);
      break;
    case WEATHER_ICON_RAIN:
      icon = SHOWERS;
      color = matrix->color565(80, 180, 255);
      break;
    case WEATHER_ICON_SNOW:
      icon = SNOW;
      color = matrix->color565(200, 240, 255);
      break;
    case WEATHER_ICON_STORM:
      icon = STORM;
      color = matrix->color565(255, 180, 0);
      break;
    default:
      return;
  }

  drawWeatherValue64(matrix, icon, x, y, color);
}

}  // namespace weather_icons

#endif

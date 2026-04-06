#ifndef CLOCK_CONFIG_H
#define CLOCK_CONFIG_H

inline constexpr char WIFI_SSID[] = "your-ssid";
inline constexpr char WIFI_PASSWORD[] = "your-password";
inline constexpr char TIME_ZONE[] = "CET-1CEST,M3.5.0/2,M10.5.0/3";
inline constexpr char NTP_SERVER_1[] = "pool.ntp.org";
inline constexpr char NTP_SERVER_2[] = "time.nist.gov";
inline constexpr float WEATHER_LATITUDE = 48.4739f;
inline constexpr float WEATHER_LONGITUDE = 2.7049f;
inline constexpr unsigned long WEATHER_REFRESH_INTERVAL_MS = 15UL * 60UL * 1000UL;

#endif

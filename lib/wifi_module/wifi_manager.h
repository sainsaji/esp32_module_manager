#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

// WiFi Configuration - Update these with your credentials
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// NTP Configuration
extern const char* NTP_SERVER;
extern const long GMT_OFFSET_SEC;
extern const int DAYLIGHT_OFFSET_SEC;

// WiFi Status Variables
extern bool wifi_enabled;
extern unsigned long last_wifi_check;
extern const unsigned long WIFI_CHECK_INTERVAL;
extern int wifi_reconnect_attempts;
extern const int MAX_RECONNECT_ATTEMPTS;

// WiFi Management Functions
void setup_wifi();
void check_wifi_status();
void connect_wifi();
void disconnect_wifi();
void show_wifi_status();
void setup_time();
void show_system_info();
void show_wifi_screen();

// WiFi Command Handlers
bool handle_wifi_command(char cmd);

// LVGL UI integration
#ifdef __cplusplus
extern "C" {
#endif

void create_wifi_screen();

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H

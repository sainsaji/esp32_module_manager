#include "wifi_manager.h"
#include "modules.h"
#include <WiFiClient.h>
#include <time.h>
#include <lvgl.h>
#include "ui_lvgl.h"
#include "system_info_screen.h"

extern int current_module;

// WiFi Configuration - Update these with your credentials
const char* WIFI_SSID = "Wi-Fi";
const char* WIFI_PASSWORD = "Mywifi#123";

// NTP Configuration
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 0;          // Adjust for your timezone
const int DAYLIGHT_OFFSET_SEC = 3600;   // Adjust for daylight saving

// WiFi Status Variables
bool wifi_enabled = true;
unsigned long last_wifi_check = 0;
const unsigned long WIFI_CHECK_INTERVAL = 10000;  // Check WiFi every 10 seconds
int wifi_reconnect_attempts = 0;
const int MAX_RECONNECT_ATTEMPTS = 3;

static lv_obj_t* screen_wifi;

lv_obj_t* label_status;
lv_obj_t* label_ip;
lv_obj_t* sig_strenght;
lv_obj_t* gateway;
lv_obj_t* MAC;
lv_obj_t* btn_reconnect;
lv_obj_t* btn_disconnect;

void update_wifi_status_ui();
void connect_wifi();
void disconnect_wifi();
void clear_wifi_info_labels();
void create_wifi_screen();
void show_wifi_info(bool toSerial, bool toUI);
void show_wifi_screen();
void setup_wifi();
void check_wifi_status();
void show_wifi_status();
void setup_time();
void show_system_info();
bool handle_wifi_command(char cmd);
bool wait_until(std::function<bool()> condition, int max_attempts, int delay_ms);

lv_obj_t* create_label(lv_obj_t* parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);

// Event callbacks
void reconnect_event_cb(lv_event_t* e);
void disconnect_event_cb(lv_event_t* e);
void reconnect_event_cb(lv_event_t* e) {
    wifi_enabled = true;
    wifi_reconnect_attempts = 0;
    connect_wifi();
    update_wifi_status_ui();
}

void disconnect_event_cb(lv_event_t* e) {
    disconnect_wifi();
    update_wifi_status_ui();
}

lv_obj_t* create_label(lv_obj_t* parent, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_align(label, align, x_ofs, y_ofs);
    return label;
}

lv_obj_t* create_button(lv_obj_t* parent, const char* text, lv_event_cb_t callback) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10); // Centered
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, NULL);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);

    return btn;
}


void create_wifi_screen() {
    screen_wifi = lv_obj_create(NULL);

    lv_obj_t* title = lv_label_create(screen_wifi);
    lv_label_set_text(title, "📡 WiFi Info");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    label_status = create_label(screen_wifi, LV_ALIGN_TOP_LEFT, 10, 40);
    label_ip = create_label(screen_wifi, LV_ALIGN_TOP_LEFT, 10, 70);
    sig_strenght = create_label(screen_wifi, LV_ALIGN_TOP_LEFT, 10, 100);
    gateway = create_label(screen_wifi, LV_ALIGN_TOP_LEFT, 10, 130);
    MAC = create_label(screen_wifi, LV_ALIGN_TOP_LEFT, 10, 160);

    btn_reconnect = create_button(screen_wifi, "🔁 Reconnect", reconnect_event_cb);
    btn_disconnect = create_button(screen_wifi, "❌ Disconnect", disconnect_event_cb);

    // Back button to go to main screen
    lv_obj_t* btn_back = lv_btn_create(screen_wifi);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_event_cb(btn_back, [](lv_event_t* e) {
        lv_scr_load(screen_main);  // screen_main should be defined in ui_lvgl.cpp
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t* lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, "🔙 Back");

    update_wifi_status_ui();
}

void clear_wifi_info_labels() {
    lv_label_set_text(label_ip, "");
    lv_label_set_text(sig_strenght, "");
    lv_label_set_text(gateway, "");
    lv_label_set_text(MAC, "");
}

void show_wifi_info(bool toSerial, bool toUI) {
    String ip     = WiFi.localIP().toString();
    String rssi   = String(WiFi.RSSI());
    String gate   = WiFi.gatewayIP().toString();
    String mac    = WiFi.macAddress();

    if (toSerial) {
        Serial.printf("📍 IP Address: %s\n", ip.c_str());
        Serial.printf("📶 Signal Strength: %s dBm\n", rssi.c_str());
        Serial.printf("🌐 Gateway: %s\n", gate.c_str());
        Serial.printf("🎭 MAC Address: %s\n", mac.c_str());
    }

    if (toUI) {
        lv_label_set_text_fmt(label_ip, "IP: %s", ip.c_str());
        lv_label_set_text_fmt(sig_strenght, "📶 Signal: %s dBm", rssi.c_str());
        lv_label_set_text_fmt(gateway, "🌐 Gateway: %s", gate.c_str());
        lv_label_set_text_fmt(MAC, "🎭 MAC: %s", mac.c_str());
    }
}

void update_wifi_status_ui() {
    if (!wifi_enabled) {
        lv_label_set_text(label_status, "❌ WiFi Disabled");
        clear_wifi_info_labels();
    } else if (WiFi.status() != WL_CONNECTED) {
        lv_label_set_text(label_status, "⚠️ Not Connected");
        lv_label_set_text_fmt(label_ip, "Reconnect attempts: %d/%d", wifi_reconnect_attempts, MAX_RECONNECT_ATTEMPTS);
        clear_wifi_info_labels();
    } else {
        show_wifi_info(false, true);
    }
        // Show/hide buttons based on connection status
    if (WiFi.status() == WL_CONNECTED) {
        lv_obj_add_flag(btn_reconnect, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btn_disconnect, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(btn_disconnect, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(btn_reconnect, LV_OBJ_FLAG_HIDDEN);
    }

}




void show_wifi_screen() {
    if (!screen_wifi) {
        create_wifi_screen();
    }
    lv_scr_load(screen_wifi);
}

void setup_wifi() {
    if (!wifi_enabled) return;

    Serial.println("\n📡 Initializing WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    connect_wifi();

    // ✅ If WiFi screen is already created, sync UI
    if (screen_wifi) {
        update_wifi_status_ui();
    }
}



bool wait_until(std::function<bool()> condition, int max_attempts, int delay_ms = 1000) {
    for (int i = 0; i < max_attempts; ++i) {
        if (condition()) return true;
        delay(delay_ms);
        Serial.print(".");
    }
    return false;
}



void connect_wifi() {
    if (!wifi_enabled) return;
    
    Serial.printf("🔌 Connecting to WiFi network: %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    bool connected = wait_until([]() {
        return WiFi.status() == WL_CONNECTED;
    }, 20, 500);
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" ✅ Connected!");
        show_wifi_info(true, false);
        
        // Setup time synchronization
        setup_time();
        
        wifi_reconnect_attempts = 0;
    } else {
        Serial.println(" ❌ Failed!");
        Serial.printf("❌ WiFi Status: %d\n", WiFi.status());
        wifi_reconnect_attempts++;
    }
}

void disconnect_wifi() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("📡 Disconnecting from WiFi...");
        WiFi.disconnect();
        delay(1000);
        Serial.println("✅ WiFi disconnected");
    }
    wifi_enabled = false;
}

void check_wifi_status() {
    if (!wifi_enabled) return;
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf("⚠️  WiFi disconnected (Status: %d)\n", WiFi.status());
        
        if (wifi_reconnect_attempts < MAX_RECONNECT_ATTEMPTS) {
            Serial.println("🔄 Attempting to reconnect...");
            connect_wifi();
        } else {
            Serial.println("❌ Max reconnection attempts reached. WiFi disabled.");
            Serial.println("💡 Use 'w' command to manually reconnect.");
            wifi_enabled = false;
        }
    }
}

void show_wifi_status() {
    Serial.println("\n📡 WiFi Status Information");
    Serial.println("==========================");
    
    if (!wifi_enabled) {
        Serial.println("Status: ❌ WiFi Disabled");
        return;
    }
    
    Serial.printf("Status: %s\n", (WiFi.status() == WL_CONNECTED) ? "✅ Connected" : "❌ Disconnected");
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("Subnet Mask: %s\n", WiFi.subnetMask().toString().c_str());
        Serial.printf("DNS 1: %s\n", WiFi.dnsIP(0).toString().c_str());
        Serial.printf("DNS 2: %s\n", WiFi.dnsIP(1).toString().c_str());
        Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
        Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
        Serial.printf("Channel: %d\n", WiFi.channel());
        
        // Show time if available
        time_t now;
        time(&now);
        if (now > 1000000000) {  // Valid timestamp
            Serial.printf("Current Time: %s", ctime(&now));
        }
    } else {
        Serial.printf("Reconnect Attempts: %d/%d\n", wifi_reconnect_attempts, MAX_RECONNECT_ATTEMPTS);
    }
}

void setup_time() {
    if (WiFi.status() != WL_CONNECTED) return;
    
    Serial.println("🕒 Synchronizing time with NTP server...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    // Wait for time synchronization
    int attempts = 0;
    bool time_synced = wait_until([]() {
        return time(nullptr) > 1000000000;
    }, 10);

    
    if (time(nullptr) > 1000000000) {
        Serial.println(" ✅ Time synchronized!");
        time_t now;
        time(&now);
        Serial.printf("📅 Current time: %s", ctime(&now));
    } else {
        Serial.println(" ❌ Time synchronization failed");
    }
}


bool handle_wifi_command(char cmd) {
    switch (cmd) {
        case 'w': case 'W':
            show_wifi_status();
            return true;
            
        case 'c': case 'C':
            wifi_enabled = true;
            wifi_reconnect_attempts = 0;
            connect_wifi();
            return true;
            
        case 'd': case 'D':
            disconnect_wifi();
            return true;
            
        case 't': case 'T':
            if (WiFi.status() == WL_CONNECTED) {
                setup_time();
            } else {
                Serial.println("❌ WiFi not connected. Connect first with 'c' command.");
            }
            return true;
        
        default:
            return false;  // Command not handled
    }
}
#include "wifi_manager.h"
#include "modules.h"
#include <WiFiClient.h>
#include <time.h>

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

void setup_wifi() {
    if (!wifi_enabled) return;
    
    Serial.println("\n📡 Initializing WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    connect_wifi();
}

void connect_wifi() {
    if (!wifi_enabled) return;
    
    Serial.printf("🔌 Connecting to WiFi network: %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" ✅ Connected!");
        Serial.printf("📍 IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("📶 Signal Strength: %d dBm\n", WiFi.RSSI());
        Serial.printf("🌐 Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("🎭 MAC Address: %s\n", WiFi.macAddress().c_str());
        
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
    while (time(nullptr) < 1000000000 && attempts < 10) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (time(nullptr) > 1000000000) {
        Serial.println(" ✅ Time synchronized!");
        time_t now;
        time(&now);
        Serial.printf("📅 Current time: %s", ctime(&now));
    } else {
        Serial.println(" ❌ Time synchronization failed");
    }
}

void show_system_info() {
    Serial.println("\n🖥️  System Information");
    Serial.println("======================");
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Largest Free Block: %d bytes\n", ESP.getMaxAllocHeap());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("Uptime: %lu ms\n", millis());
    
    // Show current WASM module
    if (current_module >= 0) {
        Serial.printf("Active WASM Module: %s\n", modules[current_module].name);
    } else {
        Serial.println("Active WASM Module: None");
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
            
        case 'i': case 'I':
            show_system_info();
            return true;
            
        default:
            return false;  // Command not handled
    }
}
#include <Arduino.h>
#include "modules.h"
#include "wasm_runner.h"
#include "wifi_manager.h"
#include <wasm3.h>

extern int current_module;

void show_menu();
void handle_serial_input();

void setup() {
    Serial.begin(115200);
    delay(1000);
    while (!Serial) {}

    Serial.println("\n🎉 ESP32 WASM3 Multi-Module System with WiFi");
    Serial.println("==============================================");
    Serial.println(
        String("Wasm3 v") + M3_VERSION + " (" + M3_ARCH + "), build " + __DATE__ + " " + __TIME__
    );
    
    // Initialize WiFi
    setup_wifi();
    
    show_menu();
}

void loop() {
    handle_serial_input();
    
    // Check WiFi status periodically
    if (wifi_enabled && (millis() - last_wifi_check > WIFI_CHECK_INTERVAL)) {
        check_wifi_status();
        last_wifi_check = millis();
    }
    
    delay(100);
}

void show_menu() {
    Serial.println("\n🔧 WASM Module Selector & System Control");
    Serial.println("========================================");
    
    // WASM Modules
    Serial.println("📦 WASM Modules:");
    for (int i = 0; i < NUM_MODULES; i++) {
        Serial.printf("  %d. %s (%d bytes)\n", i + 1, modules[i].name, modules[i].size);
    }
    
    // System Commands
    Serial.println("\n⚡ System Commands:");
    Serial.println("  s. Stop current module");
    Serial.println("  m. Show this menu");
    Serial.println("  r. Restart ESP32");
    Serial.println("  i. Show system information");
    
    // WiFi Commands
    Serial.println("\n📡 WiFi Commands:");
    Serial.println("  w. Show WiFi status");
    Serial.println("  c. Connect/Reconnect WiFi");
    Serial.println("  d. Disconnect WiFi");
    Serial.println("  t. Sync time with NTP");
    
    // Status indicators
    Serial.printf("\n📊 Current Status: WASM[%s] WiFi[%s]\n", 
                  (current_module >= 0) ? modules[current_module].name : "None",
                  (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected");
    
    Serial.print("Select option: ");
}

void handle_serial_input() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 0) return;

        char cmd = input.charAt(0);
        
        // Try to handle WiFi commands first
        if (handle_wifi_command(cmd)) {
            return;  // Command was handled by WiFi manager
        }
        
        // Handle WASM and system commands
        switch (cmd) {
            case '1': case '2': case '3':
                start_module(cmd - '1');
                break;
                
            case 's': case 'S':
                stop_current_module();
                show_menu();
                break;
                
            case 'm': case 'M':
                show_menu();
                break;
                
            case 'r': case 'R':
                Serial.println("🔄 Restarting ESP32...");
                delay(1000);
                ESP.restart();
                break;
                
            default:
                Serial.printf("❌ Unknown command '%c'\n", cmd);
                show_menu();
                break;
        }
    }
}

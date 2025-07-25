#include <Arduino.h>
#include "modules.h"
#include "wasm_runner.h"
#include "wifi_manager.h"
#include <wasm3.h>
#include <Preferences.h>

extern int current_module;
Preferences preferences;

void show_menu();
void handle_serial_input();
void handle_module_management();
void save_module_list();
void load_module_list();
String get_user_input(const char* prompt);  // New function for safe input

void setup() {
    Serial.begin(115200);
    delay(1000);
    while (!Serial) {}

    Serial.println("\n🎉 ESP32 WASM3 Dynamic Module Loader");
    Serial.println("=====================================");
    Serial.println(
        String("Wasm3 v") + M3_VERSION + " (" + M3_ARCH + "), build " + __DATE__ + " " + __TIME__
    );
    
    // Initialize preferences
    preferences.begin("wasm-loader", false);
    
    // Initialize modules
    init_modules();
    load_module_list();  // Load saved module list if exists
    
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
    Serial.println("\n🔧 WASM Dynamic Module Loader");
    Serial.println("=============================");
    
    // List modules
    list_modules();
    
    // Module Commands
    Serial.println("\n📦 Module Commands:");
    Serial.println("  1-9. Run module (if loaded)");
    Serial.println("  l.   Load/Download module");
    Serial.println("  a.   Add new module URL");
    Serial.println("  x.   Remove module");
    Serial.println("  s.   Stop current module");
    Serial.println("  z.   Clear all modules");
    
    // System Commands
    Serial.println("\n⚡ System Commands:");
    Serial.println("  m.   Show this menu");
    Serial.println("  r.   Restart ESP32");
    Serial.println("  i.   Show system information");
    
    // WiFi Commands
    Serial.println("\n📡 WiFi Commands:");
    Serial.println("  w.   Show WiFi status");
    Serial.println("  c.   Connect/Reconnect WiFi");
    Serial.println("  d.   Disconnect WiFi");
    
    // Status indicators
    Serial.printf("\n📊 Status: WASM[%s] WiFi[%s]\n", 
                  (current_module >= 0 && !modules[current_module].name.isEmpty()) 
                    ? modules[current_module].name.c_str() : "None",
                  (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected");
    
    Serial.print("\nSelect option: ");
}

// New function to safely get user input without interference
String get_user_input(const char* prompt) {
    // Clear any pending serial data
    while (Serial.available()) {
        Serial.read();
    }
    
    // Show prompt
    Serial.print(prompt);
    
    String input = "";
    bool done = false;
    
    while (!done) {
        if (Serial.available()) {
            char c = Serial.read();
            
            if (c == '\n' || c == '\r') {
                // Enter pressed
                done = true;
                Serial.println();  // New line after input
            } else if (c == 8 || c == 127) {
                // Backspace
                if (input.length() > 0) {
                    input.remove(input.length() - 1);
                    Serial.print("\b \b");  // Erase character on terminal
                }
            } else if (c >= 32 && c <= 126) {
                // Printable character
                input += c;
                Serial.print(c);  // Echo character
            }
        }
        delay(10);
    }
    
    input.trim();
    return input;
}

void handle_serial_input() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 0) return;

        char cmd = input.charAt(0);
        
        // Try to handle WiFi commands first
        if (handle_wifi_command(cmd)) {
            return;
        }
        
        // Handle module and system commands
        switch (cmd) {
            case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                start_module(cmd - '1');
                break;
                
            case 'l': case 'L':
                handle_module_management();
                break;
                
            case 'a': case 'A': {
                Serial.println("\n📝 Add New Module");
                
                // Use the new safe input function
                String name = get_user_input("Enter module name: ");
                if (name.isEmpty()) {
                    Serial.println("❌ Module name cannot be empty");
                    show_menu();
                    break;
                }
                
                String url = get_user_input("Enter module URL: ");
                if (url.isEmpty()) {
                    Serial.println("❌ Module URL cannot be empty");
                    show_menu();
                    break;
                }
                
                if (add_module(name, url)) {
                    save_module_list();
                    Serial.println("✅ Module added successfully!");
                }
                delay(1000);
                show_menu();
                break;
            }
                
            case 'x': case 'X': {
                String num = get_user_input("\nEnter module number to remove: ");
                int index = num.toInt() - 1;
                if (remove_module(index)) {
                    save_module_list();
                    Serial.println("✅ Module removed successfully!");
                } else {
                    Serial.println("❌ Failed to remove module");
                }
                delay(1000);
                show_menu();
                break;
            }
                
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

            case 'z': case 'Z': { 
                Serial.println("\n⚠️  Clear All Modules");
                String confirm = get_user_input("Are you sure? Type 'yes' to confirm: ");
                if (confirm == "yes") {
                    // Clear all modules
                    init_modules();  // This clears the array
                    save_module_list();  // Save empty list
                    Serial.println("✅ All modules cleared!");
                } else {
                    Serial.println("❌ Cancelled");
                }
                delay(1000);
                show_menu();
                break;
            }
                            
            default:
                Serial.printf("❌ Unknown command '%c'\n", cmd);
                show_menu();
                break;
        }
    }
}

void handle_module_management() {
    String input = get_user_input("\nEnter module number to download: ");
    int index = input.toInt() - 1;
    
    if (index >= 0 && index < MAX_MODULES && !modules[index].name.isEmpty()) {
        Serial.println("\n📥 Starting download...");
        if (download_module(index)) {
            Serial.println("✅ Module downloaded successfully!");
        } else {
            Serial.println("❌ Failed to download module");
        }
    } else {
        Serial.println("❌ Invalid module number");
    }
    
    delay(2000);
    show_menu();
}

void save_module_list() {
    // Save module list to preferences
    preferences.clear();
    int saved = 0;
    for (int i = 0; i < MAX_MODULES; i++) {
        if (!modules[i].name.isEmpty()) {
            String key = "mod" + String(saved);
            String value = modules[i].name + "|" + modules[i].url;
            preferences.putString(key.c_str(), value);
            saved++;
        }
    }
    preferences.putInt("count", saved);
    Serial.printf("💾 Saved %d modules to preferences\n", saved);
}

void load_module_list() {
    int count = preferences.getInt("count", 0);
    Serial.printf("📂 Loading %d saved modules\n", count);
    
    for (int i = 0; i < count; i++) {
        String key = "mod" + String(i);
        String value = preferences.getString(key.c_str(), "");
        if (!value.isEmpty()) {
            int sep = value.indexOf('|');
            if (sep > 0) {
                String name = value.substring(0, sep);
                String url = value.substring(sep + 1);
                add_module(name, url);
            }
        }
    }
}
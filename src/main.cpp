#include <Arduino.h>
#include "modules.h"
#include "wasm_runner.h"
#include <wasm3.h>

extern int current_module;

void show_menu();
void handle_serial_input();

void setup() {
    Serial.begin(115200);
    delay(1000);
    while (!Serial) {}

    Serial.println("\n🎉 ESP32 WASM3 Multi-Module System");
    Serial.println(
    String("Wasm3 v") + M3_VERSION + " (" + M3_ARCH + "), build " + __DATE__ + " " + __TIME__
);

    show_menu();
}

void loop() {
    handle_serial_input();
    delay(100);
}

void show_menu() {
    Serial.println("\n🔧 WASM Module Selector\n=");
    for (int i = 0; i < NUM_MODULES; i++) {
        Serial.printf("%d. %s (%d bytes)\n", i + 1, modules[i].name, modules[i].size);
    }
    Serial.println("s. Stop current module");
    Serial.println("m. Show this menu");
    Serial.println("r. Restart ESP32");
    Serial.print("Select option (1-" + String(NUM_MODULES) + "): ");
}

void handle_serial_input() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        if (input.length() == 0) return;

        char cmd = input.charAt(0);
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

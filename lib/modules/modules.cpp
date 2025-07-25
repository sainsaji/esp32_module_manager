#include "modules.h"
#include <HTTPClient.h>
#include <WiFi.h>

// Dynamic module storage
WasmModule modules[10];  // Support up to 10 dynamic modules
const int MAX_MODULES = 10;
int num_loaded_modules = 0;

void init_modules() {
    // Clear all modules first
    for (int i = 0; i < MAX_MODULES; i++) {
        modules[i].name = "";
        modules[i].url = "";
        modules[i].bytecode = nullptr;
        modules[i].size = 0;
        modules[i].loaded = false;
    }
    num_loaded_modules = 0;
    
    // Don't add any default modules here!
    // They will be loaded from preferences or added manually
}

void cleanup_modules() {
    for (int i = 0; i < MAX_MODULES; i++) {
        if (modules[i].bytecode != nullptr) {
            free(modules[i].bytecode);
            modules[i].bytecode = nullptr;
        }
        modules[i].loaded = false;
        modules[i].size = 0;
    }
    num_loaded_modules = 0;
}

bool add_module(const String& name, const String& url) {
    if (num_loaded_modules >= MAX_MODULES) {
        Serial.println("❌ Module list is full");
        return false;
    }
    
    // Check for duplicates
    for (int i = 0; i < MAX_MODULES; i++) {
        if (!modules[i].name.isEmpty() && 
            modules[i].name == name && 
            modules[i].url == url) {
            Serial.printf("⚠️  Module '%s' already exists\n", name.c_str());
            return false;
        }
    }
    
    // Find first empty slot
    for (int i = 0; i < MAX_MODULES; i++) {
        if (modules[i].name.isEmpty()) {
            modules[i].name = name;
            modules[i].url = url;
            modules[i].bytecode = nullptr;
            modules[i].size = 0;
            modules[i].loaded = false;
            num_loaded_modules++;
            Serial.printf("✅ Added module: %s\n", name.c_str());
            return true;
        }
    }
    return false;
}

bool remove_module(int index) {
    if (index < 0 || index >= MAX_MODULES || modules[index].name.isEmpty()) {
        return false;
    }
    
    if (modules[index].bytecode != nullptr) {
        free(modules[index].bytecode);
    }
    
    modules[index].name = "";
    modules[index].url = "";
    modules[index].bytecode = nullptr;
    modules[index].size = 0;
    modules[index].loaded = false;
    num_loaded_modules--;
    
    Serial.printf("✅ Removed module at index %d\n", index);
    return true;
}

bool download_module(int index) {
    if (index < 0 || index >= MAX_MODULES || modules[index].name.isEmpty()) {
        Serial.println("❌ Invalid module index");
        return false;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi not connected");
        return false;
    }
    
    WasmModule* mod = &modules[index];
    
    // Clean up previous data if exists
    if (mod->bytecode != nullptr) {
        free(mod->bytecode);
        mod->bytecode = nullptr;
    }
    
    Serial.printf("📥 Downloading: %s from %s\n", mod->name.c_str(), mod->url.c_str());
    
    HTTPClient http;
    http.begin(mod->url);
    http.setTimeout(30000); // 30 second timeout
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        int len = http.getSize();
        
        if (len <= 0) {
            Serial.println("❌ Invalid content length");
            http.end();
            return false;
        }
        
        Serial.printf("📦 Module size: %d bytes\n", len);
        
        // Allocate memory for module
        mod->bytecode = (uint8_t*)malloc(len);
        if (mod->bytecode == nullptr) {
            Serial.println("❌ Failed to allocate memory");
            http.end();
            return false;
        }
        
        // Download data
        WiFiClient* stream = http.getStreamPtr();
        size_t written = 0;
        uint8_t buff[512];
        
        while (http.connected() && (written < len)) {
            size_t available = stream->available();
            if (available) {
                int c = stream->readBytes(buff, ((available > sizeof(buff)) ? sizeof(buff) : available));
                if (c > 0) {
                    memcpy(mod->bytecode + written, buff, c);
                    written += c;
                    
                    // Progress indicator
                    if (written % 4096 == 0 || written == len) {
                        Serial.printf("📊 Progress: %d/%d bytes (%.1f%%)\r", 
                                    written, len, (float)written/len * 100);
                    }
                }
            }
            delay(1);
        }
        Serial.println();
        
        if (written == len) {
            mod->size = len;
            mod->loaded = true;
            Serial.printf("✅ Successfully downloaded %s (%d bytes)\n", mod->name.c_str(), len);
            http.end();
            return true;
        } else {
            Serial.printf("❌ Download incomplete: %d/%d bytes\n", written, len);
            free(mod->bytecode);
            mod->bytecode = nullptr;
        }
    } else {
        Serial.printf("❌ HTTP error: %d\n", httpCode);
    }
    
    http.end();
    return false;
}

void list_modules() {
    Serial.println("\n📦 Available WASM Modules:");
    Serial.println("==========================");
    
    int count = 0;
    for (int i = 0; i < MAX_MODULES; i++) {
        if (!modules[i].name.isEmpty()) {
            count++;
            Serial.printf("%d. %s %s\n", count,  // Use count, not i+1
                         modules[i].name.c_str(),
                         modules[i].loaded ? "✅ (loaded)" : "⏳ (not loaded)");
            Serial.printf("   URL: %s\n", modules[i].url.c_str());
            if (modules[i].loaded) {
                Serial.printf("   Size: %d bytes\n", modules[i].size);
            }
        }
    }
    
    if (count == 0) {
        Serial.println("No modules configured. Use 'a' to add modules.");
    }
}
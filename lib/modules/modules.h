#pragma once
#include <Arduino.h>

struct WasmModule {
    String name;
    String url;
    uint8_t* bytecode;
    size_t size;
    bool loaded;
};

extern WasmModule modules[];
extern const int MAX_MODULES;
extern int num_loaded_modules;

// Module management functions
void init_modules();
void cleanup_modules();
bool add_module(const String& name, const String& url);
bool remove_module(int index);
bool download_module(int index);
void list_modules();
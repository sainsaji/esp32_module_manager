#pragma once

struct WasmModule {
    const char* name;
    const unsigned char* bytecode;
    unsigned int size;
};

extern WasmModule modules[];
extern const int NUM_MODULES;

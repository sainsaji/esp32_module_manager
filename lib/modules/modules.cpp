#include "modules.h"
#include "app_wasm.h"

WasmModule modules[] = {
    {"select module 1", module1, module1_len},
    {"select module 2", module2, module2_len},
    {"select module 3", module3, module3_len}
};

const int NUM_MODULES = sizeof(modules) / sizeof(modules[0]);

#include <Arduino.h>
#include <wasm3.h>
#include <m3_env.h>
#include "modules.h"
#include "wasm_bindings.h"
#include "wasm_runner.h"

#define WASM_STACK_SLOTS    1024
#define NATIVE_STACK_SIZE   (32*1024)
#define WASM_MEMORY_LIMIT   4096

extern TaskHandle_t wasm_task_handle;
extern int current_module;

TaskHandle_t wasm_task_handle = NULL;
int current_module = -1;

void wasm_task(void* parameter)
{
    int module_id = (int)(intptr_t)parameter;

    if (module_id < 0 || module_id >= MAX_MODULES || modules[module_id].name.isEmpty()) {
        Serial.println("❌ Invalid module ID");
        vTaskDelete(NULL);
        return;
    }

    WasmModule* mod = &modules[module_id];
    
    // Check if module is loaded
    if (!mod->loaded || mod->bytecode == nullptr) {
        Serial.println("❌ Module not loaded. Please download it first.");
        vTaskDelete(NULL);
        return;
    }
    Serial.printf("🚀 Starting WASM Module: %s\n", mod->name);

    IM3Environment env = m3_NewEnvironment();
    if (!env) {
        Serial.println("❌ Failed to create environment");
        vTaskDelete(NULL);
        return;
    }

    IM3Runtime runtime = m3_NewRuntime(env, WASM_STACK_SLOTS, NULL);
    if (!runtime) {
        Serial.println("❌ Failed to create runtime");
        vTaskDelete(NULL);
        return;
    }

    runtime->memoryLimit = WASM_MEMORY_LIMIT;

    IM3Module module;
    M3Result result = m3_ParseModule(env, &module, mod->bytecode, mod->size);
    if (result) {
        Serial.print("❌ ParseModule failed: ");
        Serial.println(result);
        vTaskDelete(NULL);
        return;
    }

    result = m3_LoadModule(runtime, module);
    if (result) {
        Serial.print("❌ LoadModule failed: ");
        Serial.println(result);
        vTaskDelete(NULL);
        return;
    }

    result = LinkArduino(runtime);
    if (result) {
        Serial.print("❌ LinkArduino failed: ");
        Serial.println(result);
        vTaskDelete(NULL);
        return;
    }

    IM3Function f;
    result = m3_FindFunction(&f, runtime, "_start");
    if (result) {
        Serial.print("❌ Cannot find _start function: ");
        Serial.println(result);
        vTaskDelete(NULL);
        return;
    }

    Serial.printf("✅ Running module: %s\n", mod->name);
    Serial.println("📝 Send 's' to stop and return to menu");

    result = m3_CallV(f);

    if (result) {
        Serial.print("❌ WASM execution error: ");
        Serial.println(result);
    }

    Serial.printf("🏁 Module '%s' stopped\n", mod->name);
    vTaskDelete(NULL);
}

void stop_current_module()
{
    if (wasm_task_handle != NULL) {
        Serial.println("🛑 Stopping current module...");
        vTaskDelete(wasm_task_handle);
        wasm_task_handle = NULL;
        current_module = -1;
        Serial.println("✅ Module stopped");
    } else {
        Serial.println("ℹ️  No module is currently running");
    }
}

void start_module(int module_id)
{
    if (module_id < 0 || module_id >= MAX_MODULES || modules[module_id].name.isEmpty()) {
        Serial.println("❌ Invalid module selection");
        return;
    }
    
    if (!modules[module_id].loaded) {
        Serial.println("❌ Module not loaded. Download it first with 'l' command.");
        return;
    }

    stop_current_module();

    current_module = module_id;
    xTaskCreate(&wasm_task,
                modules[module_id].name.c_str(),
                NATIVE_STACK_SIZE,
                (void*)(intptr_t)module_id,
                5,
                &wasm_task_handle);

    Serial.printf("🚀 Started module: %s\n", modules[module_id].name.c_str());
}

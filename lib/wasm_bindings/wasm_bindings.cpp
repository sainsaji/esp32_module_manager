#include <Arduino.h>
#include <wasm3.h>
#include <m3_env.h>
#include "wasm_bindings.h"

#define DEFINE_WASM_API(name, args_block, native_call) \
    m3ApiRawFunction(name) { \
        args_block \
        native_call; \
        m3ApiSuccess(); \
    }

DEFINE_WASM_API(
  m3_arduino_delay,
  m3ApiGetArg(uint32_t, ms),
  delay(ms)
)

DEFINE_WASM_API(
  m3_arduino_print,
  m3ApiGetArgMem(const char*, str),
  Serial.print(str)
)

#define ARDUINO_WASM_BINDINGS \
    X("arduino_delay",      "v(i)",   m3_arduino_delay) \
    X("arduino_print",      "v(*)",   m3_arduino_print) \

M3Result LinkArduino(IM3Runtime runtime)
{
    IM3Module module = runtime->modules;
    const char* env = "env";

    Serial.println("🔗 Linking Arduino functions...");

    M3Result result = m3Err_none;

    #define X(name, sig, fn) \
        result = m3_LinkRawFunction(module, env, name, sig, &fn); \
        if (result == m3Err_none) Serial.println("✓ " name " linked"); \
        else Serial.printf("❌ Failed to link " name ": %s\n", result);

    ARDUINO_WASM_BINDINGS
    #undef X

    return m3Err_none;
}

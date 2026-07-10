#pragma once
#include <stdint.h>

// --- System Boot Reasons ---
#define ME_BOOT_REASON_NONE         0
#define ME_BOOT_REASON_NORMAL       1
#define ME_BOOT_REASON_SETUP        2  // Power button held for 3 seconds
#define ME_BOOT_REASON_FORCE_RESET  3  // POWER button held for 5s (or BOOT held for 200ms)
#define ME_BOOT_REASON_WDT          4  // WDT: High-Performance (HP) core locked up/deadlocked
#define ME_BOOT_REASON_THERMAL      5  // Emergency Thermal Reset: Junction temperature exceeded limit

// --- Status Flags (Shared Memory - NOT UL P-specific) ---
// On ESP32-S3, these are plain global variables since the ME runs as a FreeRTOS task.
// The "ulp_" prefix is kept for API compatibility with bios_core and power_mgmt.

extern volatile uint32_t ulp_me_is_enabled;
extern volatile uint32_t ulp_me_hp_is_awake;
extern volatile uint32_t ulp_me_boot_reason;
extern volatile uint32_t ulp_me_wdt_counter;

// --- SchedUtil & Thermal Protection Metrics ---
extern volatile uint32_t ulp_me_cpu_load;
extern volatile uint32_t ulp_me_max_freq;
extern volatile uint32_t ulp_me_target_freq;
extern volatile uint32_t ulp_me_temperature;
extern volatile uint32_t ulp_me_throttle_temp;
extern volatile uint32_t ulp_me_emergency_temp;
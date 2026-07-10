#include "management_engine.h"
#include "me_shared.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ME_HOST";
static bool me_active = false;

// On ESP32-S3, shared variables are plain globals (no ULP RAM needed)
volatile uint32_t ulp_me_is_enabled   = 1;
volatile uint32_t ulp_me_hp_is_awake  = 0;
volatile uint32_t ulp_me_boot_reason  = ME_BOOT_REASON_NONE;
volatile uint32_t ulp_me_wdt_counter  = 0;

volatile uint32_t ulp_me_cpu_load       = 0;
volatile uint32_t ulp_me_max_freq       = 240;
volatile uint32_t ulp_me_target_freq    = 240;
volatile uint32_t ulp_me_temperature    = 0;
volatile uint32_t ulp_me_throttle_temp  = 55;
volatile uint32_t ulp_me_emergency_temp = 75;

extern void management_engine_s3_task(void *pvParameters);

static bool is_valid_boot_reason(uint32_t r) {
    return r == ME_BOOT_REASON_NONE        ||
    r == ME_BOOT_REASON_NORMAL      ||
    r == ME_BOOT_REASON_SETUP       ||
    r == ME_BOOT_REASON_FORCE_RESET ||
    r == ME_BOOT_REASON_WDT         ||
    r == ME_BOOT_REASON_THERMAL;
}

void management_engine_init(bool is_enabled, bool is_cold_boot) {
    if (!is_enabled) {
        ESP_LOGW(TAG, "Management Engine is DISABLED. System running in Legacy Mode.");
        return;
    }

    if (!is_cold_boot) {
        ESP_LOGI(TAG, "ME already running (wakeup path). Skipping re-deploy.");
        me_active = true;
        return;
    }

    ESP_LOGI(TAG, "Starting OPENS3 Management Engine on ESP32-S3 (HP FreeRTOS Task)...");

    ulp_me_is_enabled  = 1;
    ulp_me_boot_reason = ME_BOOT_REASON_NONE;

    BaseType_t ret = xTaskCreate(
        management_engine_s3_task,
        "me_s3_task",
        4096,
        NULL,
        8,
        NULL
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create ME S3 task!");
        return;
    }

    me_active = true;
    ESP_LOGI(TAG, "ME is Active. WDT & Button logic running on HP core task.");
}

bool management_engine_check_force_shutdown(void) {
    return false;
}

uint32_t management_engine_get_boot_reason(void) {
    uint32_t r = ulp_me_boot_reason;
    if (!is_valid_boot_reason(r)) {
        ESP_LOGW(TAG, "Invalid boot reason (0x%08lX) — treating as NONE", (unsigned long)r);
        return ME_BOOT_REASON_NONE;
    }
    return r;
}

void management_engine_clear_boot_reason(void) {
    ulp_me_boot_reason = ME_BOOT_REASON_NONE;
}

void management_engine_pet_watchdog(void) {
    if (!me_active) return;
    ulp_me_wdt_counter++;
}
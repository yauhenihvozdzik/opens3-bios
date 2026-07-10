/**
 * ESP32-S3 Management Engine — FreeRTOS Task
 *
 * On ESP32-C6, the Management Engine runs on the dedicated LP-Core (ULP RISC-V).
 * On ESP32-S3, there is no LP-Core, so the ME runs as a high-priority FreeRTOS
 * task on the main Xtensa cores. The API surface (me_shared.h variables) is
 * preserved for drop-in compatibility with bios_core and power_mgmt.
 */

#include <stdint.h>
#include <stdbool.h>
#include "me_shared.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ME_S3";

// ─── GPIO MAPPING (ESP32-S3 compatible pin assignment) ─────────────────────────
// These pins are available on most ESP32-S3 dev boards.
#define PIN_BTN_GND     3
#define PIN_BTN_SENSE   4

// ─── WDT & TIMING CONSTANTS ───────────────────────────────────────────────────
#define ME_WDT_TIMEOUT_MS   15000
#define ME_LOOP_DELAY_MS    10

// ─── ESP32-S3 FREQUENCY PRESETS (MHz) ─────────────────────────────────────────
#define FREQ_LOW     80
#define FREQ_MED     160
#define FREQ_HIGH    240

// ─── HARDWARE RESET ────────────────────────────────────────────────────────────
static void me_hw_reset(uint32_t reason) {
    ulp_me_boot_reason = reason;
    vTaskDelay(pdMS_TO_TICKS(50));
    esp_restart();
}

// ─── MAIN MANAGEMENT ENGINE TASK ───────────────────────────────────────────────
void management_engine_s3_task(void *pvParameters) {
    ESP_LOGI(TAG, "ME Task started on ESP32-S3 HP core");

    // Configure power button GPIOs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_BTN_GND) | (1ULL << PIN_BTN_SENSE),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_set_direction(PIN_BTN_GND, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_BTN_GND, 0);
    gpio_set_pull_mode(PIN_BTN_SENSE, GPIO_PULLUP_ONLY);

    // ─── Runtime state ────────────────────────────────────────────────────────
    uint32_t pwr_press_timer  = 0;
    uint32_t wdt_last_counter = 0;
    uint32_t wdt_stale_ms     = 0;
    uint32_t sched_loop_timer = 0;
    uint32_t me_uptime_ms     = 0;

    while (1) {
        if (ulp_me_is_enabled == 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        me_uptime_ms += ME_LOOP_DELAY_MS;

        // ─── 1. POWER BUTTON MANAGEMENT ───────────────────────────────────────
        if (gpio_get_level(PIN_BTN_SENSE) == 0) {
            if (pwr_press_timer < 10000) pwr_press_timer += ME_LOOP_DELAY_MS;
            if (ulp_me_hp_is_awake == 1 && pwr_press_timer >= 5000) {
                ESP_LOGW(TAG, "Force reset (5s power hold)");
                me_hw_reset(ME_BOOT_REASON_FORCE_RESET);
            }
        } else {
            if (pwr_press_timer > 0) {
                if (ulp_me_hp_is_awake == 0) {
                    if (pwr_press_timer >= 3000) {
                        ulp_me_boot_reason = ME_BOOT_REASON_SETUP;
                    } else if (pwr_press_timer >= 50) {
                        ulp_me_boot_reason = ME_BOOT_REASON_NORMAL;
                    }
                }
                pwr_press_timer = 0;
            }
        }

        // ─── 2. SOFTWARE WATCHDOG ─────────────────────────────────────────────
        if (ulp_me_hp_is_awake == 1) {
            if (ulp_me_wdt_counter != wdt_last_counter) {
                wdt_last_counter = ulp_me_wdt_counter;
                wdt_stale_ms = 0;
            } else {
                wdt_stale_ms += ME_LOOP_DELAY_MS;
                if (wdt_stale_ms >= ME_WDT_TIMEOUT_MS) {
                    ESP_LOGE(TAG, "WDT timeout! HP core unresponsive");
                    me_hw_reset(ME_BOOT_REASON_WDT);
                }
            }
        } else {
            wdt_last_counter = ulp_me_wdt_counter;
            wdt_stale_ms = 0;
        }

        // ─── 3. FREQUENCY SCALING & THERMAL ───────────────────────────────────
        sched_loop_timer += ME_LOOP_DELAY_MS;
        if (sched_loop_timer >= 100) {
            sched_loop_timer = 0;

            if (me_uptime_ms > 3000 &&
                ulp_me_temperature >= ulp_me_emergency_temp &&
                ulp_me_hp_is_awake == 1) {
                ESP_LOGE(TAG, "THERMAL SHUTDOWN! Temp=%lu", (unsigned long)ulp_me_temperature);
                me_hw_reset(ME_BOOT_REASON_THERMAL);
            }

            uint32_t target = FREQ_LOW;
            if (ulp_me_temperature >= ulp_me_throttle_temp) {
                target = FREQ_LOW;
            } else {
                if (ulp_me_cpu_load > 75)      target = FREQ_HIGH;
                else if (ulp_me_cpu_load > 30) target = FREQ_MED;
                else                           target = FREQ_LOW;
            }

            if (target > ulp_me_max_freq) target = ulp_me_max_freq;
            ulp_me_target_freq = target;
        }

        vTaskDelay(pdMS_TO_TICKS(ME_LOOP_DELAY_MS));
    }
}
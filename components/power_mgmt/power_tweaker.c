#include <stdio.h>
#include "power_tweaker.h"
#include "esp_pm.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

static const char *TAG = "AI_TWEAKER";

extern void power_governor_start(void);

void power_tweaker_apply_bios_settings(void) {
    cpu_freq_t freq;
    cpu_governor_t gov;
    bod_level_t bod;

    nvram_get_cpu_freq(&freq);
    nvram_get_cpu_governor(&gov);
    nvram_get_bod_level(&bod);

    ESP_LOGI(TAG, "=== APPLYING HARDWARE TWEAKS ===");

    // 1. CPU Frequency & Governor
    #if CONFIG_PM_ENABLE
    if (gov == GOV_DYNAMIC) {
        ESP_LOGI(TAG, "CPU Governor: DYNAMIC");
    } else {
        esp_pm_config_t pm_config = {
            .max_freq_mhz = freq,
            .min_freq_mhz = freq,
            .light_sleep_enable = false
        };
        esp_pm_configure(&pm_config);
        ESP_LOGI(TAG, "CPU Clock locked at %d MHz", freq);
    }
    power_governor_start();
    #endif

    // 2. Brownout Detector — real hardware control via RTC_CNTL
    if (bod == BOD_DISABLED) {
        REG_CLR_BIT(RTC_CNTL_BROWN_OUT_REG, RTC_CNTL_BROWN_OUT_RST_ENA);
        ESP_LOGW(TAG, "BOD: PHYSICAL PROTECTION DISABLED!");
    } else {
        uint32_t threshold = (bod == BOD_STRICT) ? 7 : 5;
        uint32_t bod_reg = REG_READ(RTC_CNTL_BROWN_OUT_REG);
        // Clear threshold bits, set new threshold
        bod_reg &= ~(RTC_CNTL_BROWN_OUT_DET_M | RTC_CNTL_BROWN_OUT_RST_ENA);
        bod_reg |= (threshold << RTC_CNTL_BROWN_OUT_DET_S) | RTC_CNTL_BROWN_OUT_RST_ENA;
        REG_WRITE(RTC_CNTL_BROWN_OUT_REG, bod_reg);
        ESP_LOGI(TAG, "BOD Level: %s (Threshold %lu, ~%s)",
                 bod == BOD_STRICT ? "STRICT" : "RELAXED",
                 (unsigned long)threshold,
                 bod == BOD_STRICT ? "2.8V" : "2.5V");
    }

    ESP_LOGI(TAG, "=================================");
}
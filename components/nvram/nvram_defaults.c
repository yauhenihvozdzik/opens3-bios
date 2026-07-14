#include "esp_log.h"
#include "nvs_flash.h"
#include "nvram.h"
#include "nvram_schema.h"

static const char *TAG = "NVRAM_DEF";

esp_err_t nvram_load_defaults(void) {
    ESP_LOGW(TAG, "Initiating BIOS Factory Reset (Clear CMOS)...");

    esp_err_t err = nvs_flash_erase();
    if (err != ESP_OK) { ESP_LOGE(TAG, "Failed to erase NVS: %s", esp_err_to_name(err)); return err; }

    err = nvram_init();
    if (err != ESP_OK) return err;

    // Basic
    nvram_set_dc_loss_action(DC_LOSS_POWER_OFF);
    nvram_set_post_led_mode(POST_LED_ENABLED);
    nvram_set_aura_mode(AURA_RAINBOW);
    nvram_set_aura_brightness(128);

    // AI Tweaker
    nvram_set_cpu_freq(CPU_FREQ_240MHZ);
    nvram_set_cpu_governor(GOV_DYNAMIC);
    nvram_set_bod_level(BOD_STRICT);
    nvram_set_cpu_cores(CPU_CORES_2);
    nvram_set_psram_speed(PSRAM_120MHZ);
    nvram_set_psram_mode(PSRAM_OCTAL);
    nvram_set_thermal_limits(VAL_TEMP_THROTTLE_DEFAULT, VAL_TEMP_EMERGENCY_DEFAULT);

    // ME
    nvram_set_me_state(ME_ENABLED);
    nvram_set_wdt_timeout(WDT_TIMEOUT_DEFAULT);

    // Sleep
    nvram_set_sleep_wakeup(SLEEP_DISABLED);
    nvram_set_sleep_timer(SLEEP_TIMER_DEFAULT);

    // Network
    nvram_set_wifi_sta_config("", "");
    nvram_set_pxe_url(VAL_PXE_URL_DEFAULT);
    nvram_set_hostname(VAL_HOSTNAME_DEFAULT);
    nvram_set_wifi_tx_power(WIFI_TX_POWER_DEFAULT);
    nvram_set_ap_channel(AP_CHANNEL_DEFAULT);

    // Boot
    nvram_set_boot_order(BOOT_ORDER_FLASH);
    nvram_set_boot_timeout(BOOT_TIMEOUT_DEFAULT);

    // Console
    nvram_set_uart_baud(UART_BAUD_115200);
    nvram_set_usb_serial(USB_SERIAL_ENABLED);

    // Security
    nvram_set_secure_boot(SECURE_BOOT_DISABLED);
    nvram_set_flash_encrypt(FLASH_ENCRYPT_DISABLED);

    // GPIO
    nvram_set_gpio_drive(GPIO_DRIVE_20MA);

    ESP_LOGI(TAG, "BIOS Setup Defaults loaded (28 params).");
    return ESP_OK;
}

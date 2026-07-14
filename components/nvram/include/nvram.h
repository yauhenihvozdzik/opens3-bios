#pragma once

#include "esp_err.h"
#include "nvram_schema.h"

/**
 * @brief Initializes the Non-Volatile Storage (NVS) flash subsystem.
 */
esp_err_t nvram_init(void);

/**
 * @brief Restores factory system defaults (Clear CMOS behavior).
 */
esp_err_t nvram_load_defaults(void);

/**
 * @brief Manages the system power mode behavior upon DC loss (DC Loss Action).
 */
esp_err_t nvram_get_dc_loss_action(dc_loss_action_t *action);
esp_err_t nvram_set_dc_loss_action(dc_loss_action_t action);

/**
 * @brief Controls the POST diagnostic LED status mode.
 */
esp_err_t nvram_get_post_led_mode(post_led_mode_t *mode);
esp_err_t nvram_set_post_led_mode(post_led_mode_t mode);

/**
 * @brief Manages Wi-Fi station network credentials (SSID and Password).
 */
esp_err_t nvram_get_wifi_sta_config(char* ssid, size_t ssid_len, char* pass, size_t pass_len);
esp_err_t nvram_set_wifi_sta_config(const char* ssid, const char* pass);

/**
 * @brief Manages the PXE boot server URL configuration interface.
 */
esp_err_t nvram_get_pxe_url(char* url, size_t max_len); // <--- Reads target PXE URL from NVRAM
esp_err_t nvram_set_pxe_url(const char* url);          // <--- Writes target PXE URL to NVRAM

/**
 * @brief Controls the active Aura Sync RGB lighting effect.
 */
esp_err_t nvram_get_aura_mode(aura_mode_t *mode);
esp_err_t nvram_set_aura_mode(aura_mode_t mode);

/**
 * @brief Adjusts the Aura Sync RGB lighting brightness intensity (0-255).
 */
esp_err_t nvram_get_aura_brightness(uint8_t *brightness);
esp_err_t nvram_set_aura_brightness(uint8_t brightness);

/**
 * @brief Manages the run-state configuration of the Management Engine (LP-Core).
 */
esp_err_t nvram_get_me_state(me_state_t *state);
esp_err_t nvram_set_me_state(me_state_t state);

// ============================================================================
// AI TWEAKER (OVERCLOCKING & POWER CONTROLS)
// ============================================================================

/**
 * @brief Configures the upper frequency boundary of the CPU core.
 */
esp_err_t nvram_get_cpu_freq(cpu_freq_t *freq);
esp_err_t nvram_set_cpu_freq(cpu_freq_t freq);

/**
 * @brief Controls the active CPU dynamic power scaling governor profile.
 */
esp_err_t nvram_get_cpu_governor(cpu_governor_t *gov);
esp_err_t nvram_set_cpu_governor(cpu_governor_t gov);

/**
 * @brief Configures the hardware Brownout Detector (BOD) voltage protection level.
 */
esp_err_t nvram_get_bod_level(bod_level_t *level);
esp_err_t nvram_set_bod_level(bod_level_t level);

/**
 * @brief Manages junction temperature thresholds for thermal throttling and emergency shutdowns.
 */
esp_err_t nvram_get_thermal_limits(uint8_t *throttle, uint8_t *emergency);
esp_err_t nvram_set_thermal_limits(uint8_t throttle, uint8_t emergency);


esp_err_t nvram_get_bios_update_state(bios_update_state_t *state);
esp_err_t nvram_set_bios_update_state(bios_update_state_t state);


// ====== EXPANDED S3 PARAMETERS ======
esp_err_t nvram_get_cpu_cores(cpu_cores_t *cores);
esp_err_t nvram_set_cpu_cores(cpu_cores_t cores);
esp_err_t nvram_get_psram_speed(psram_speed_t *speed);
esp_err_t nvram_set_psram_speed(psram_speed_t speed);
esp_err_t nvram_get_psram_mode(psram_mode_t *mode);
esp_err_t nvram_set_psram_mode(psram_mode_t mode);
esp_err_t nvram_get_wdt_timeout(uint8_t *timeout);
esp_err_t nvram_set_wdt_timeout(uint8_t timeout);
esp_err_t nvram_get_sleep_wakeup(sleep_wakeup_t *wakeup);
esp_err_t nvram_set_sleep_wakeup(sleep_wakeup_t wakeup);
esp_err_t nvram_get_sleep_timer(uint16_t *timer);
esp_err_t nvram_set_sleep_timer(uint16_t timer);
esp_err_t nvram_get_wifi_tx_power(uint8_t *power);
esp_err_t nvram_set_wifi_tx_power(uint8_t power);
esp_err_t nvram_get_ap_channel(uint8_t *channel);
esp_err_t nvram_set_ap_channel(uint8_t channel);
esp_err_t nvram_get_hostname(char* name, size_t max_len);
esp_err_t nvram_set_hostname(const char* name);
esp_err_t nvram_get_boot_order(boot_order_t *order);
esp_err_t nvram_set_boot_order(boot_order_t order);
esp_err_t nvram_get_boot_timeout(uint8_t *timeout);
esp_err_t nvram_set_boot_timeout(uint8_t timeout);
esp_err_t nvram_get_uart_baud(uart_baud_t *baud);
esp_err_t nvram_set_uart_baud(uart_baud_t baud);
esp_err_t nvram_get_usb_serial(usb_serial_t *enabled);
esp_err_t nvram_set_usb_serial(usb_serial_t enabled);
esp_err_t nvram_get_secure_boot(secure_boot_t *enabled);
esp_err_t nvram_set_secure_boot(secure_boot_t enabled);
esp_err_t nvram_get_flash_encrypt(flash_encrypt_t *enabled);
esp_err_t nvram_set_flash_encrypt(flash_encrypt_t enabled);
esp_err_t nvram_get_gpio_drive(gpio_drive_t *drive);
esp_err_t nvram_set_gpio_drive(gpio_drive_t drive);

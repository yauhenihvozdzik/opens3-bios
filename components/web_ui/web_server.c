#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_partition.h>
#include <esp_flash.h>
#include <esp_heap_caps.h>
#include "esp_rom_sys.h"     // ROM functions for diagnostic clock queries
#include <nvs.h>
#include "cJSON.h"
#include "nvram.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "me_shared.h"   // Included to access temperature and active ME state

static const char *TAG = "WEB_UI";

// Embedded index.html boundaries (auto-generated via EMBED_FILES in CMake)
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

/**
 * @brief GET / - Serves the primary BIOS configuration HTML document.
 */
static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
}

/**
 * @brief GET /api/sysinfo - Provides real-time hardware status metrics.
 */
static esp_err_t sysinfo_get_handler(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();

    // 1. Flash Storage Information
    uint32_t flash_size = 0;
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        cJSON_AddNumberToObject(root, "flash_total_mb", flash_size / (1024 * 1024));
    }

    // 2. RAM Sizing Statistics (Internal SRAM)
    uint32_t ram_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    uint32_t ram_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    cJSON_AddNumberToObject(root, "ram_total_kb", ram_total / 1024);
    cJSON_AddNumberToObject(root, "ram_free_kb", ram_free / 1024);

    // 3. Real-time CPU Frequency (MHz) and Junction Temperature
    cJSON_AddNumberToObject(root, "cpu_real_mhz", esp_rom_get_cpu_ticks_per_us());
    cJSON_AddNumberToObject(root, "cpu_temp_c", ulp_me_temperature);
    cJSON_AddNumberToObject(root, "cpu_cores", CONFIG_FREERTOS_NUMBER_OF_CORES);
    cJSON_AddNumberToObject(root, "uptime_sec", xTaskGetTickCount() * portTICK_PERIOD_MS / 1000);
    cJSON_AddNumberToObject(root, "wdt_timeout", 0); // placeholder, updated below if ME is enabled
    cJSON_AddNumberToObject(root, "psram_size_mb", 0); // will try to get PSRAM info

    // Try to get PSRAM size if available
    size_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    cJSON_AddNumberToObject(root, "psram_size_mb", (int)(psram_size / (1024 * 1024)));

    // FS free space (approximate: total opens3_fs partition size minus head/tail offset)
    // Since we can not call opens3_fs functions directly here, we estimate from the partition table
    const esp_partition_t *fs_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0xff, "opens3_fs");
    int fs_free_kb = 0;
    if (fs_part) {
        // Rough estimate: we use the partition size as free space for display purposes
        // Real free space would require calling fs_get_free_space()
        fs_free_kb = (int)(fs_part->size / 1024);
    }
    cJSON_AddNumberToObject(root, "fs_free_kb", fs_free_kb);

    // WDT timeout placeholder (real value filled by ME task in shared memory)
    // For now, report the configured value (static)
    uint8_t wdt_to = WDT_TIMEOUT_DEFAULT;
    cJSON_AddNumberToObject(root, "wdt_timeout", wdt_to);

    // 4. Partition Table Scan (Storage Mapping)
    cJSON *parts = cJSON_AddArrayToObject(root, "partitions");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (it != NULL) {
        const esp_partition_t *p = esp_partition_get(it);
        cJSON *part = cJSON_CreateObject();
        cJSON_AddStringToObject(part, "name", p->label);
        cJSON_AddNumberToObject(part, "offset", p->address);
        cJSON_AddNumberToObject(part, "size_kb", p->size / 1024);
        cJSON_AddItemToArray(parts, part);
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);

    // 5. NVS (CMOS) Statistics
    nvs_stats_t nvs_stats;
    if (nvs_get_stats(NULL, &nvs_stats) == ESP_OK) {
        cJSON_AddNumberToObject(root, "nvs_used_entries", nvs_stats.used_entries);
        cJSON_AddNumberToObject(root, "nvs_free_entries", nvs_stats.free_entries);
    }

    const char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);

    free((void*)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief GET /api/settings - Reads current system configurations to populate the Web UI form.
 */
static esp_err_t settings_get_handler(httpd_req_t *req) {
    dc_loss_action_t dc_action;
    post_led_mode_t led_mode;
    aura_mode_t aura_mode;
    uint8_t aura_br;
    cpu_freq_t cpu_freq;
    cpu_governor_t cpu_gov;
    bod_level_t bod_lvl;
    me_state_t me_state;
    uint8_t temp_th;
    uint8_t temp_em;
    char ssid[33] = {0};
    char pass[65] = {0};
    char pxe_url[PXE_URL_MAX_LEN + 1] = {0}; // Buffer for PXE boot URL target
    cpu_cores_t cpu_cores;
    psram_speed_t psram_speed;
    psram_mode_t psram_mode;
    uint8_t wdt_timeout;
    sleep_wakeup_t sleep_wakeup;
    uint16_t sleep_timer;
    uint8_t wifi_tx_power;
    uint8_t ap_channel;
    char hostname[HOSTNAME_MAX_LEN + 1] = {0};
    boot_order_t boot_order;
    uint8_t boot_timeout;
    uart_baud_t uart_baud;
    usb_serial_t usb_serial;
    secure_boot_t secure_boot;
    flash_encrypt_t flash_encrypt;
    gpio_drive_t gpio_drive;

    nvram_get_dc_loss_action(&dc_action);
    nvram_get_post_led_mode(&led_mode);
    nvram_get_wifi_sta_config(ssid, sizeof(ssid), pass, sizeof(pass));
    nvram_get_pxe_url(pxe_url, sizeof(pxe_url)); // Retrieve target URL from NVRAM
    nvram_get_aura_mode(&aura_mode);
    nvram_get_aura_brightness(&aura_br);
    nvram_get_cpu_freq(&cpu_freq);
    nvram_get_cpu_governor(&cpu_gov);
    nvram_get_bod_level(&bod_lvl);
    nvram_get_me_state(&me_state);
    nvram_get_thermal_limits(&temp_th, &temp_em);
    nvram_get_cpu_cores(&cpu_cores);
    nvram_get_psram_speed(&psram_speed);
    nvram_get_psram_mode(&psram_mode);
    nvram_get_wdt_timeout(&wdt_timeout);
    nvram_get_sleep_wakeup(&sleep_wakeup);
    nvram_get_sleep_timer(&sleep_timer);
    nvram_get_wifi_tx_power(&wifi_tx_power);
    nvram_get_ap_channel(&ap_channel);
    nvram_get_hostname(hostname, sizeof(hostname));
    nvram_get_boot_order(&boot_order);
    nvram_get_boot_timeout(&boot_timeout);
    nvram_get_uart_baud(&uart_baud);
    nvram_get_usb_serial(&usb_serial);
    nvram_get_secure_boot(&secure_boot);
    nvram_get_flash_encrypt(&flash_encrypt);
    nvram_get_gpio_drive(&gpio_drive);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "dc_loss", dc_action);
    cJSON_AddNumberToObject(root, "post_led", led_mode);
    cJSON_AddStringToObject(root, "wifi_ssid", ssid);
    cJSON_AddStringToObject(root, "wifi_pass", pass);
    cJSON_AddStringToObject(root, "pxe_url", pxe_url);     // Send PXE target configuration to Web UI
    cJSON_AddNumberToObject(root, "aura_mode", aura_mode);
    cJSON_AddNumberToObject(root, "aura_brightness", aura_br);
    cJSON_AddNumberToObject(root, "cpu_freq", cpu_freq);
    cJSON_AddNumberToObject(root, "cpu_gov", cpu_gov);
    cJSON_AddNumberToObject(root, "bod_lvl", bod_lvl);
    cJSON_AddNumberToObject(root, "me_enable", me_state);
    cJSON_AddNumberToObject(root, "temp_th", temp_th);
    cJSON_AddNumberToObject(root, "temp_em", temp_em);
    cJSON_AddNumberToObject(root, "cpu_cores", (int)cpu_cores);
    cJSON_AddNumberToObject(root, "psram_speed", (int)psram_speed);
    cJSON_AddNumberToObject(root, "psram_mode", (int)psram_mode);
    cJSON_AddNumberToObject(root, "wdt_timeout", wdt_timeout);
    cJSON_AddNumberToObject(root, "sleep_wakeup", (int)sleep_wakeup);
    cJSON_AddNumberToObject(root, "sleep_timer", sleep_timer);
    cJSON_AddNumberToObject(root, "wifi_tx_power", wifi_tx_power);
    cJSON_AddNumberToObject(root, "ap_channel", ap_channel);
    cJSON_AddStringToObject(root, "hostname", hostname);
    cJSON_AddNumberToObject(root, "boot_order", (int)boot_order);
    cJSON_AddNumberToObject(root, "boot_timeout", boot_timeout);
    cJSON_AddNumberToObject(root, "uart_baud", (int)uart_baud);
    cJSON_AddNumberToObject(root, "usb_serial", (int)usb_serial);
    cJSON_AddNumberToObject(root, "secure_boot", (int)secure_boot);
    cJSON_AddNumberToObject(root, "flash_encrypt", (int)flash_encrypt);
    cJSON_AddNumberToObject(root, "gpio_drive", (int)gpio_drive);

    const char *json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);

    free((void*)json_str);
    cJSON_Delete(root);
    return ESP_OK;
}

/**
 * @brief POST /api/settings - Parses and saves incoming system configurations to NVRAM.
 */
static esp_err_t settings_post_handler(httpd_req_t *req) {
    char buf[1024];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (root) {
        cJSON *dc = cJSON_GetObjectItem(root, "dc_loss");
        cJSON *led = cJSON_GetObjectItem(root, "post_led");
        cJSON *ssid = cJSON_GetObjectItem(root, "wifi_ssid");
        cJSON *pass = cJSON_GetObjectItem(root, "wifi_pass");
        cJSON *pxe = cJSON_GetObjectItem(root, "pxe_url");   // Retrieve target URL from client payload
        cJSON *aura = cJSON_GetObjectItem(root, "aura_mode");
        cJSON *br = cJSON_GetObjectItem(root, "aura_brightness");
        cJSON *freq = cJSON_GetObjectItem(root, "cpu_freq");
        cJSON *gov = cJSON_GetObjectItem(root, "cpu_gov");
        cJSON *bod = cJSON_GetObjectItem(root, "bod_lvl");
        cJSON *me = cJSON_GetObjectItem(root, "me_enable");
        cJSON *th = cJSON_GetObjectItem(root, "temp_th");
        cJSON *em = cJSON_GetObjectItem(root, "temp_em");
        cJSON *cores = cJSON_GetObjectItem(root, "cpu_cores");
        cJSON *pspd = cJSON_GetObjectItem(root, "psram_speed");
        cJSON *pspm = cJSON_GetObjectItem(root, "psram_mode");
        cJSON *wdtt = cJSON_GetObjectItem(root, "wdt_timeout");
        cJSON *slpw = cJSON_GetObjectItem(root, "sleep_wakeup");
        cJSON *slpt = cJSON_GetObjectItem(root, "sleep_timer");
        cJSON *host = cJSON_GetObjectItem(root, "hostname");
        cJSON *txpw = cJSON_GetObjectItem(root, "wifi_tx_power");
        cJSON *apch = cJSON_GetObjectItem(root, "ap_channel");
        cJSON *btor = cJSON_GetObjectItem(root, "boot_order");
        cJSON *btto = cJSON_GetObjectItem(root, "boot_timeout");
        cJSON *ubd = cJSON_GetObjectItem(root, "uart_baud");
        cJSON *usbs = cJSON_GetObjectItem(root, "usb_serial");
        cJSON *sb = cJSON_GetObjectItem(root, "secure_boot");
        cJSON *fe = cJSON_GetObjectItem(root, "flash_encrypt");
        cJSON *gdrv = cJSON_GetObjectItem(root, "gpio_drive");

        if (dc) nvram_set_dc_loss_action((dc_loss_action_t)dc->valueint);
        if (led) nvram_set_post_led_mode((post_led_mode_t)led->valueint);
        if (aura) nvram_set_aura_mode((aura_mode_t)aura->valueint);
        if (br) nvram_set_aura_brightness((uint8_t)br->valueint);
        if (freq) nvram_set_cpu_freq((cpu_freq_t)freq->valueint);
        if (gov) nvram_set_cpu_governor((cpu_governor_t)gov->valueint);
        if (bod) nvram_set_bod_level((bod_level_t)bod->valueint);
        if (me)  nvram_set_me_state((me_state_t)me->valueint);
        if(cores) nvram_set_cpu_cores((cpu_cores_t)cores->valueint);
        if(pspd) nvram_set_psram_speed((psram_speed_t)pspd->valueint);
        if(pspm) nvram_set_psram_mode((psram_mode_t)pspm->valueint);
        if(wdtt) nvram_set_wdt_timeout((uint8_t)wdtt->valueint);
        if(slpw) nvram_set_sleep_wakeup((sleep_wakeup_t)slpw->valueint);
        if(slpt) nvram_set_sleep_timer((uint16_t)slpt->valueint);
        if(txpw) nvram_set_wifi_tx_power((uint8_t)txpw->valueint);
        if(apch) nvram_set_ap_channel((uint8_t)apch->valueint);
        if(host && host->valuestring) nvram_set_hostname(host->valuestring);
        if(btor) nvram_set_boot_order((boot_order_t)btor->valueint);
        if(btto) nvram_set_boot_timeout((uint8_t)btto->valueint);
        if(ubd) nvram_set_uart_baud((uart_baud_t)ubd->valueint);
        if(usbs) nvram_set_usb_serial((usb_serial_t)usbs->valueint);
        if(sb) nvram_set_secure_boot((secure_boot_t)sb->valueint);
        if(fe) nvram_set_flash_encrypt((flash_encrypt_t)fe->valueint);
        if(gdrv) nvram_set_gpio_drive((gpio_drive_t)gdrv->valueint);

        // Commit thermal safety and throttling limits to NVRAM
        if (th && em) {
            nvram_set_thermal_limits((uint8_t)th->valueint, (uint8_t)em->valueint);
        }

        if (ssid && pass) {
            nvram_set_wifi_sta_config(ssid->valuestring, pass->valuestring);
        }

        // Commit PXE URL configurations to NVRAM
        if (pxe && pxe->valuestring) {
            nvram_set_pxe_url(pxe->valuestring);
        }

        cJSON_Delete(root);
        ESP_LOGI(TAG, "BIOS Settings synchronized with NVRAM");
        return httpd_resp_sendstr(req, "OK");
    }

    httpd_resp_send_500(req);
    return ESP_FAIL;
}

/**
 * @brief POST /api/trigger_ota - Enables pending BIOS wireless firmware OTA update state flag and restarts system.
 */
static esp_err_t trigger_ota_handler(httpd_req_t *req) {
    ESP_LOGW(TAG, "BIOS Network Update (OTA) triggered from Web UI!");

    esp_err_t err = nvram_set_bios_update_state(BIOS_UPDATE_PENDING);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set BIOS update pending state: %s", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "OK");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Allow transient response frame to propagate network buffers
    esp_restart();
    return ESP_OK;
}

static esp_err_t reboot_handler(httpd_req_t *req) {
    ESP_LOGW(TAG, "Reboot command received...");
    httpd_resp_sendstr(req, "Rebooting...");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
    return ESP_OK;
}

static esp_err_t defaults_handler(httpd_req_t *req) {
    ESP_LOGE(TAG, "Load Defaults command received...");
    nvram_load_defaults();
    return httpd_resp_sendstr(req, "OK");
}

void web_ui_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.stack_size = 10240; // Provide sufficient stack space allocation for complex cJSON parsing

    ESP_LOGI(TAG, "Starting OPENS3 Web Interface on port %d", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // HTTP Router Handler Registrations
        httpd_uri_t uri_root = { .uri = "/", .method = HTTP_GET, .handler = root_handler };
        httpd_register_uri_handler(server, &uri_root);

        httpd_uri_t uri_get = { .uri = "/api/settings", .method = HTTP_GET, .handler = settings_get_handler };
        httpd_register_uri_handler(server, &uri_get);

        httpd_uri_t uri_post = { .uri = "/api/settings", .method = HTTP_POST, .handler = settings_post_handler };
        httpd_register_uri_handler(server, &uri_post);

        httpd_uri_t uri_sys = { .uri = "/api/sysinfo", .method = HTTP_GET, .handler = sysinfo_get_handler };
        httpd_register_uri_handler(server, &uri_sys);

        httpd_uri_t uri_reboot = { .uri = "/api/reboot", .method = HTTP_POST, .handler = reboot_handler };
        httpd_register_uri_handler(server, &uri_reboot);

        httpd_uri_t uri_def = { .uri = "/api/defaults", .method = HTTP_POST, .handler = defaults_handler };
        httpd_register_uri_handler(server, &uri_def);

        // Register router handler to initiate wireless BIOS firmware update OTA
        httpd_uri_t uri_trig_ota = { .uri = "/api/trigger_ota", .method = HTTP_POST, .handler = trigger_ota_handler };
        httpd_register_uri_handler(server, &uri_trig_ota);
    }
}

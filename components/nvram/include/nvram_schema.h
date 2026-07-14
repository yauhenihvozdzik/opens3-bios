#pragma once
#include <stdint.h>

#define BIOS_NVS_NAMESPACE "bios_cfg"

// ============================================================================
// 1. BASIC CONFIGURATION
// ============================================================================
typedef enum { DC_LOSS_POWER_OFF = 0, DC_LOSS_POWER_ON = 1 } dc_loss_action_t;
typedef enum { POST_LED_DISABLED = 0, POST_LED_ENABLED = 1 } post_led_mode_t;
typedef enum { AURA_DISABLED = 0, AURA_RAINBOW = 1, AURA_BLUE = 2 } aura_mode_t;

// ============================================================================
// 2. CPU & POWER TUNING (AI TWEaker)
// ============================================================================
typedef enum { CPU_FREQ_80MHZ = 80, CPU_FREQ_160MHZ = 160, CPU_FREQ_240MHZ = 240 } cpu_freq_t;
typedef enum { GOV_PERFORMANCE = 0, GOV_DYNAMIC = 1 } cpu_governor_t;
typedef enum { BOD_STRICT = 0, BOD_RELAXED = 1, BOD_DISABLED = 2 } bod_level_t;
typedef enum { CPU_CORES_1 = 1, CPU_CORES_2 = 2 } cpu_cores_t;
typedef enum { PSRAM_80MHZ = 80, PSRAM_120MHZ = 120 } psram_speed_t;
typedef enum { PSRAM_DISABLE = 0, PSRAM_QUAD = 1, PSRAM_OCTAL = 2 } psram_mode_t;

// ============================================================================
// 3. MANAGEMENT ENGINE
// ============================================================================
typedef enum { ME_DISABLED = 0, ME_ENABLED = 1 } me_state_t;
typedef enum { BIOS_UPDATE_NONE = 0, BIOS_UPDATE_PENDING = 1 } bios_update_state_t;
#define WDT_TIMEOUT_MIN     5
#define WDT_TIMEOUT_MAX     60
#define WDT_TIMEOUT_DEFAULT 15

// ============================================================================
// 4. SLEEP / WAKEUP
// ============================================================================
typedef enum { SLEEP_DISABLED = 0, SLEEP_TIMER = 1, SLEEP_GPIO = 2 } sleep_wakeup_t;
#define SLEEP_TIMER_MIN     5
#define SLEEP_TIMER_MAX     3600
#define SLEEP_TIMER_DEFAULT 60

// ============================================================================
// 5. NETWORK CONFIGURATION
// ============================================================================
#define WIFI_SSID_MAX_LEN   32
#define WIFI_PASS_MAX_LEN   64
#define PXE_URL_MAX_LEN     128
#define HOSTNAME_MAX_LEN    32
#define WIFI_TX_POWER_MIN   8
#define WIFI_TX_POWER_MAX   20
#define WIFI_TX_POWER_DEFAULT 20
#define AP_CHANNEL_MIN      1
#define AP_CHANNEL_MAX      13
#define AP_CHANNEL_DEFAULT  1

// ============================================================================
// 6. THERMAL LIMITS
// ============================================================================
#define VAL_TEMP_THROTTLE_DEFAULT  55
#define VAL_TEMP_EMERGENCY_DEFAULT 75

// ============================================================================
// 7. BOOT SEQUENCE
// ============================================================================
typedef enum { BOOT_ORDER_FLASH = 0, BOOT_ORDER_PXE = 1, BOOT_ORDER_SHELL = 2 } boot_order_t;
#define BOOT_TIMEOUT_MIN    0
#define BOOT_TIMEOUT_MAX    30
#define BOOT_TIMEOUT_DEFAULT 5

// ============================================================================
// 8. UART / USB CONSOLE
// ============================================================================
typedef enum { UART_BAUD_9600=9600, UART_BAUD_19200=19200, UART_BAUD_38400=38400, UART_BAUD_57600=57600, UART_BAUD_115200=115200, UART_BAUD_230400=230400, UART_BAUD_460800=460800, UART_BAUD_921600=921600 } uart_baud_t;
typedef enum { USB_SERIAL_ENABLED = 1, USB_SERIAL_DISABLED = 0 } usb_serial_t;

// ============================================================================
// 9. SECURITY
// ============================================================================
typedef enum { SECURE_BOOT_DISABLED = 0, SECURE_BOOT_ENABLED = 1 } secure_boot_t;
typedef enum { FLASH_ENCRYPT_DISABLED = 0, FLASH_ENCRYPT_ENABLED = 1 } flash_encrypt_t;

// ============================================================================
// 10. GPIO DRIVE STRENGTH
// ============================================================================
typedef enum { GPIO_DRIVE_5MA = 0, GPIO_DRIVE_10MA = 1, GPIO_DRIVE_20MA = 2, GPIO_DRIVE_40MA = 3 } gpio_drive_t;

// ============================================================================
// DEFAULTS
// ============================================================================
#define VAL_PXE_URL_DEFAULT "http://192.168.1.1:8080/payload.bin"
#define VAL_HOSTNAME_DEFAULT "OPENS3-BIOS"

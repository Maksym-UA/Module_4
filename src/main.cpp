#include <cinttypes>
#include <cstring>

#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {

// --- Configuration Constants ---
// UART1: Device communication (STM32)
constexpr uart_port_t kDeviceUart = UART_NUM_1;
constexpr gpio_num_t kDeviceTxPin = GPIO_NUM_17;
constexpr gpio_num_t kDeviceRxPin = GPIO_NUM_18;
constexpr int kDeviceBaudRate = 115200; // Must match STM32

// Hardware Pins
constexpr gpio_num_t kButtonPin = GPIO_NUM_0; // Boot Button
constexpr gpio_num_t kLedPin = GPIO_NUM_2;    // Onboard LED

// UART0: USB Serial Monitor (Debug)
constexpr uart_port_t kMonitorUart = UART_NUM_0;
constexpr int kMonitorBaudRate = 115200;

constexpr int kRxBufferSize = 256;
constexpr TickType_t kReadTimeout = pdMS_TO_TICKS(10); // Short non-blocking delay
constexpr int64_t kDebounceMs = 40;

const char *TAG = "ctrl_logic";

// --- Global State ---
bool is_blinking = false;
bool led_state = false;
int64_t last_blink_time = 0;
int64_t last_button_event_ms = 0;
bool was_button_pressed = false;

void initUart() {
    // Initialize UART1 (Connection to STM32)
    const uart_config_t device_config = {
        .baud_rate = kDeviceBaudRate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {},
    };

    ESP_ERROR_CHECK(uart_driver_install(kDeviceUart, kRxBufferSize, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(kDeviceUart, &device_config));
    ESP_ERROR_CHECK(uart_set_pin(kDeviceUart, kDeviceTxPin, kDeviceRxPin,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // UART0 is initialized by default for logging, but we can reconfig if needed.
    // We will skip explicit UART0 re-init to rely on standard IDF logging.
}

void initGpio() {
    // Configure Button (Active Low)
    gpio_reset_pin(kButtonPin);
    gpio_set_direction(kButtonPin, GPIO_MODE_INPUT);
    gpio_set_pull_mode(kButtonPin, GPIO_PULLUP_ONLY);

    // Configure LED
    gpio_reset_pin(kLedPin);
    gpio_set_direction(kLedPin, GPIO_MODE_OUTPUT);
    gpio_set_level(kLedPin, 0);
}
} // namespace

extern "C" void app_main(void) {
    initUart();
    initGpio();

    ESP_LOGI(TAG, "=== STM32 Controller Started ===");
    ESP_LOGI(TAG, "TX: GPIO%d | RX: GPIO%d | Baud: %d", kDeviceTxPin, kDeviceRxPin, kDeviceBaudRate);
    ESP_LOGI(TAG, "Press BOOT button to toggle STM32.");

    std::uint8_t rx_buffer[kRxBufferSize];

    while (true) {
        // --- 1. Read Incoming UART (STM32 -> ESP32) ---
        int rx_bytes = uart_read_bytes(kDeviceUart, rx_buffer, sizeof(rx_buffer), kReadTimeout);

        if (rx_bytes > 0) {
            // Check if we received the 'T' command
            for (int i = 0; i < rx_bytes; ++i) {
                if (rx_buffer[i] == 'T') {
                    is_blinking = !is_blinking;
                    ESP_LOGI(TAG, "Received 'T': Local Blinking is now %s", is_blinking ? "ON" : "OFF");
                }
            }
        }

        // --- 2. Read Button (ESP32 -> STM32), edge + debounce ---
        const bool is_button_pressed = (gpio_get_level(kButtonPin) == 0);
        const int64_t now_ms = esp_timer_get_time() / 1000;

        if (is_button_pressed && !was_button_pressed && (now_ms - last_button_event_ms) > kDebounceMs) {
            const char cmd = 'T';
            uart_write_bytes(kDeviceUart, &cmd, 1);
            ESP_LOGI(TAG, "Button Pressed: Sent 'T' to STM32");
            last_button_event_ms = now_ms;
        }
        was_button_pressed = is_button_pressed;

        // --- 3. Handle Blinking (Non-blocking) ---
        if (is_blinking) {
            int64_t now = esp_timer_get_time() / 1000; // Microseconds to Milliseconds
            if (now - last_blink_time > 500) {
                led_state = !led_state;
                gpio_set_level(kLedPin, led_state);
                last_blink_time = now;
            }
        } else {
            // Ensure LED is off when not blinking
            if (led_state) {
                led_state = false;
                gpio_set_level(kLedPin, 0);
            }
        }

        // Yield to watchdog
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
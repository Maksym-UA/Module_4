#include <cinttypes>

#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
// UART1: Device communication
constexpr uart_port_t kDeviceUart = UART_NUM_1;
constexpr gpio_num_t kDeviceTxPin = GPIO_NUM_17;
constexpr gpio_num_t kDeviceRxPin = GPIO_NUM_18;
constexpr int kDeviceBaudRate = 9600;

// UART0: USB Serial Monitor (uses default pins via USB)
constexpr uart_port_t kMonitorUart = UART_NUM_0;
constexpr int kMonitorBaudRate = 115200;

constexpr int kRxBufferSize = 256;
constexpr TickType_t kReadTimeout = pdMS_TO_TICKS(20);

const char *TAG = "uart_bridge";

void initUart() {
    // Initialize UART1 (device communication)
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

    // Initialize UART0 (monitor) - already initialized by IDF, but reconfigure baud if needed
    const uart_config_t monitor_config = {
        .baud_rate = kMonitorBaudRate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {},
    };

    ESP_ERROR_CHECK(uart_param_config(kMonitorUart, &monitor_config));
}
}

extern "C" void app_main(void) {
    initUart();

    ESP_LOGI(TAG, "=== UART Bridge Initialized ===");
    ESP_LOGI(TAG, "UART1 (Device): TX=GPIO17 RX=GPIO18 @ %d baud", kDeviceBaudRate);
    ESP_LOGI(TAG, "UART0 (Monitor): USB @ %d baud", kMonitorBaudRate);
    ESP_LOGI(TAG, "Data from monitor forwards to device and vice versa");

    std::uint8_t device_buffer[kRxBufferSize];
    std::uint8_t monitor_buffer[kRxBufferSize];

    while (true) {
        // Read from device (UART1) and forward to monitor (UART0)
        int device_bytes = uart_read_bytes(kDeviceUart, device_buffer, sizeof(device_buffer), kReadTimeout);
        if (device_bytes > 0) {
            ESP_LOGI(TAG, "[Device→Monitor] %d bytes received from UART1", device_bytes);
            for (int i = 0; i < device_bytes; ++i) {
                ESP_LOGI(TAG, "  Byte %d: 0x%02" PRIX8, i, device_buffer[i]);
            }
            // Forward to monitor
            uart_write_bytes(kMonitorUart, reinterpret_cast<const char *>(device_buffer), device_bytes);
        }

        // Read from monitor (UART0) and forward to device (UART1)
        int monitor_bytes = uart_read_bytes(kMonitorUart, monitor_buffer, sizeof(monitor_buffer), kReadTimeout);
        if (monitor_bytes > 0) {
            ESP_LOGI(TAG, "[Monitor→Device] %d bytes received from UART0", monitor_bytes);
            for (int i = 0; i < monitor_bytes; ++i) {
                ESP_LOGI(TAG, "  Byte %d: 0x%02" PRIX8, i, monitor_buffer[i]);
            }
            // Forward to device
            uart_write_bytes(kDeviceUart, reinterpret_cast<const char *>(monitor_buffer), monitor_bytes);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
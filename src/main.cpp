#include <Arduino.h>
#include <HardwareSerial.h>

// UART1: connection to STM32 — TX=GPIO17, RX=GPIO18
HardwareSerial stm32Serial(1);

// Hardware pins
constexpr int kButtonPin = 0;   // BOOT button (active low, built-in pull-up)
constexpr int kLedPin    = 2;   // Onboard LED
constexpr int kPinTx     = 17;  // UART1 TX → STM32 RX
constexpr int kPinRx     = 18;  // UART1 RX ← STM32 TX

// Timing
constexpr unsigned long kBlinkIntervalMs = 500;
constexpr unsigned long kDebounceMs      = 40;

// State
bool          isBlinking       = false;
bool          ledState         = false;
unsigned long lastBlinkMs      = 0;
unsigned long lastButtonMs     = 0;
bool          wasButtonPressed = false;

void setup() {
    // USB Serial Monitor
    Serial.begin(115200);

    // UART1: STM32 communication — TX=17, RX=18, 115200 8N1
    stm32Serial.begin(115200, SERIAL_8N1, kPinRx, kPinTx);

    // Button: active low with internal pull-up
    pinMode(kButtonPin, INPUT_PULLUP);

    // LED: output, start off
    pinMode(kLedPin, OUTPUT);
    digitalWrite(kLedPin, LOW);

    Serial.println("=== STM32 Controller Ready ===");
    Serial.println("TX: GPIO17 | RX: GPIO18 | 115200 baud");
    Serial.println("Press BOOT button to toggle STM32 blink.");
}

void loop() {
    // --- 1. Receive from STM32, toggle local blink on 'T' ---
    while (stm32Serial.available()) {
        const char ch = static_cast<char>(stm32Serial.read());
        if (ch == 'T') {
            isBlinking = !isBlinking;
            if (!isBlinking) {
                digitalWrite(kLedPin, LOW);
                ledState = false;
            }
            Serial.print("Received 'T' from STM32 → Blink: ");
            Serial.println(isBlinking ? "ON" : "OFF");
        }
    }

    // --- 2. Button press → send 'T' to STM32 (edge + debounce) ---
    const bool isPressed = (digitalRead(kButtonPin) == LOW);
    const unsigned long now = millis();

    if (isPressed && !wasButtonPressed && (now - lastButtonMs) >= kDebounceMs) {
        stm32Serial.print('T');
        lastButtonMs = now;
        Serial.println("Button pressed → Sent 'T' to STM32");
    }
    wasButtonPressed = isPressed;

    // --- 3. Non-blocking blink ---
    if (isBlinking) {
        if (now - lastBlinkMs >= kBlinkIntervalMs) {
            ledState = !ledState;
            digitalWrite(kLedPin, ledState ? HIGH : LOW);
            lastBlinkMs = now;
        }
    }
}
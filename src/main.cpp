#include <Arduino.h>
#include <HardwareSerial.h>

// UART1: connection to STM32 — TX=GPIO17, RX=GPIO18
HardwareSerial stm32Serial(1);


constexpr int kButtonPin = 0;   // BOOT button (active low, built-in pull-up)
constexpr int kPinTx     = 17;  // UART1 TX → STM32 RX
constexpr int kPinRx     = 18;  // UART1 RX ← STM32 TX

#ifdef RGB_BUILTIN
constexpr int kRgbPin = RGB_BUILTIN; //C++ naming convention for constants
#else
constexpr int kRgbPin = 48;
#endif


constexpr unsigned long kBlinkIntervalMs = 500;
constexpr unsigned long kDebounceMs      = 40;


bool          isBlinking       = false;
bool          ledState         = false;
unsigned long lastBlinkMs      = 0;
unsigned long lastButtonMs     = 0;
bool          wasButtonPressed = false;

//set RGB LED color (blue when on, off otherwise)
void setRgbLed(bool on) {
    if (on) {
        neopixelWrite(kRgbPin, 0, 20, 32);
    } else {
        neopixelWrite(kRgbPin, 0, 0, 0);
    }
}

void setup() {

    Serial.begin(115200);

    unsigned long serialWaitStart = millis();

    //give the USB serial connection time to come up after Serial.begin(...)
    while (!Serial && (millis() - serialWaitStart) < 2500) {
        delay(10);
    }
    delay(200);

    // UART1: STM32 communication — TX=17, RX=18, 115200 8N1
    stm32Serial.begin(115200, SERIAL_8N1, kPinRx, kPinTx);

    // Button: active low with internal pull-up
    pinMode(kButtonPin, INPUT_PULLUP);

    // RGB LED: start off
    pinMode(kRgbPin, OUTPUT);
    setRgbLed(false);

    Serial.println("=== STM32 Controller Ready ===");
    Serial.println("TX: GPIO17 | RX: GPIO18 | 115200 baud rate");
    Serial.println("Press BOOT button to toggle STM32 blink.");
}

void loop() {
    //Receive from STM32, toggle local blink on 'T'
    while (stm32Serial.available()) {
        const char ch = static_cast<char>(stm32Serial.read());
        if (ch == 'T') {
            isBlinking = !isBlinking;
            if (!isBlinking) {
                setRgbLed(false);
                ledState = false;
            }
            Serial.print("Received 'T' from STM32 → Blink: ");
            Serial.println(isBlinking ? "ON" : "OFF");
        }
    }

    //Button press → send 'T' to STM32 (edge + debounce)
    const bool isPressed = (digitalRead(kButtonPin) == LOW);
    const unsigned long now = millis();

    if (isPressed && !wasButtonPressed && (now - lastButtonMs) >= kDebounceMs) {
        stm32Serial.print('T');
        lastButtonMs = now;
        Serial.println("Button pressed → Sent 'T' to STM32");
    }
    wasButtonPressed = isPressed;

    //Non-blocking blink control
    if (isBlinking) {
        if (now - lastBlinkMs >= kBlinkIntervalMs) {
            ledState = !ledState;
            setRgbLed(ledState);
            lastBlinkMs = now;
        }
    } else if (ledState) {
        ledState = false;
        setRgbLed(false);
    }
}
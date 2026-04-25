#include <Arduino.h>
#include <Wire.h>
#include "I2CScanner.hpp"
#include "SSD1306Display.hpp"

#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9

app::I2CScanResult scanResult;
app::SSD1306Display display;


void setup() {

    Serial.begin(115200);
    unsigned long serialWaitStart = millis();
    while (!Serial && (millis() - serialWaitStart) < 2500) {
        delay(10);
    }
    delay(200);

    Serial.println("Setup start");
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    Wire.setClock(400000);

    app::scanI2CDevices(Wire, Serial, scanResult);
    display.begin();

     Serial.println("Setup complete");
}


void loop() {
    static uint32_t frame = 0;
    ++frame;

    display.renderScanResults(scanResult, frame);

    Serial.print("Display frame: ");
    Serial.println(frame);

    delay(500);
}
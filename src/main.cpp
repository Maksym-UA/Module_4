#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9

// Rotate if needed: U8G2_R0, U8G2_R1, U8G2_R2, U8G2_R3
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

void setup() {
    Serial.begin(115200);
    delay(200);

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    Wire.setClock(400000);

    u8g2.begin();
    u8g2.enableUTF8Print();

    Serial.println("SSD1306 init OK (Wire + U8g2)");
}

void loop() {
    static uint32_t frame = 0;
    ++frame;

    u8g2.clearBuffer();

    // Equivalent of your previous Linux drawing test
    u8g2.drawFrame(0, 0, 128, 64);      // drawRect border
    u8g2.drawHLine(10, 20, 50);         // horizontal line
    u8g2.drawVLine(64, 10, 40);         // vertical line
    u8g2.drawPixel(64, 32);             // single pixel

    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.setCursor(2, 62);
    u8g2.print("Frame: ");
    u8g2.print(frame);

    u8g2.sendBuffer();

    Serial.print("Display frame: ");
    Serial.println(frame);

    delay(500);
}
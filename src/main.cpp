#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9

// HW I2C constructor — pins are set via Wire.begin(), no pin args needed here
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

void scanI2CDevices() {
  uint8_t found = 0;
  Serial.println("I2C scan start...");

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      if (addr < 16) {
        Serial.print('0');
      }
      Serial.println(addr, HEX);
      found++;
    }
  }

  if (found == 0) {
    Serial.println("No I2C devices found");
  }
}

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

    scanI2CDevices();

    // Initialize the display – try primary address 0x3C, then 0x3D
   /*  bool oledOk = u8g2.begin();
    if (!oledOk) {
        Serial.println("OLED begin() failed at 0x3C, trying 0x3D...");
        u8g2.setI2CAddress(0x3D << 1);
        oledOk = u8g2.begin();
    }
    if (oledOk) {
        Serial.println("OLED init OK");
    } else {
        Serial.println("OLED init FAILED — check wiring/power/address");
    } */

    u8g2.begin();

    // Execute manual training sequence
    u8g2.sendF("c",  0xAE);          // 1. Display OFF
    u8g2.sendF("ca", 0xA8, 0x3F);    // 2. Set Multiplex for 64 rows
    u8g2.sendF("ca", 0xD3, 0x00);    // 3. Set Display Offset (none)
    u8g2.sendF("c",  0x40);          // 4. Set Start Line to row 0
    u8g2.sendF("ca", 0x20, 0x00);    // 5. Addressing Mode: Horizontal
    u8g2.sendF("c",  0xA1);          // 6. Segment Remap (Horizontally flipped)
    u8g2.sendF("c",  0xC8);          // 7. COM Scan Direction (Vertically flipped)
    u8g2.sendF("ca", 0xDA, 0x12);    // 8. COM Pins Config (Sequential=0x12 for 64px)
    u8g2.sendF("ca", 0x81, 0x7F);    // 9. Contrast (0x00 to 0xFF)
    u8g2.sendF("ca", 0xD9, 0x22);    // 10. Pre-charge Period
    u8g2.sendF("ca", 0xDB, 0x20);    // 11. VCOMH Deselect Level
    u8g2.sendF("ca", 0x8D, 0x14);    // 12. Charge Pump ENABLE (Required for 3.3V)
    u8g2.sendF("c",  0xAF);          // 13. Display ON

}

void loop() {
    static uint32_t frame = 0;
    ++frame;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(15, 25, "Module 4.2");
    u8g2.drawFrame(0, 0, 128, 64);

    u8g2.sendBuffer();

    Serial.print("Display frame: ");
    Serial.println(frame);

    delay(500);
}
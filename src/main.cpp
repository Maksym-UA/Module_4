#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9

//constructor for the SSD1306 128x64 OLED display
//use U8G2_SSD1306_128X64_NONAME_F_HW_I2C for hardware I2C
// Constructor format: Rotation, Reset Pin, Clock Pin (SCL), Data Pin (SDA)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL_PIN, /* data=*/ OLED_SDA_PIN);

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
    delay(400); // Wait for serial to initialize

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    scanI2CDevices();
    // Initialize the display
    u8g2.begin();

    //Set i2c clock speed to 100kHz
    Wire.setClock(100000);
}

void loop() {
    // Clear the display buffer
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);

    // Draw some text on the display
    u8g2.drawStr(15, 25, "Hello, World!");
    u8g2.drawStr(25, 45, "SSD1306 OLED");

    u8g2.drawFrame(0, 0, 128, 64);      // Draw a border around the screen

    // Send the buffer to the display
    u8g2.sendBuffer();

    // Wait for a while before updating the display again
    delay(2000);
}
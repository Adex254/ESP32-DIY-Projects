
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd;

const int SDA_PIN = 21;
const int SCL_PIN = 22;

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(SDA_PIN, SCL_PIN);

  int status = lcd.begin(16, 2);

  if (status != 0) {
    Serial.print("LCD error: ");
    Serial.println(status);
    hd44780::fatalError(status);
  }

  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("HELLO ESP32!");

  lcd.setCursor(0, 1);
  lcd.print("LCD TEST OK");
}

void loop() {
}

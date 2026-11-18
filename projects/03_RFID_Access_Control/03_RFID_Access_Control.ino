
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

// LCD1602 I2C pins
#define LCD_SDA 21
#define LCD_SCL 22

// RC522 SPI pins
#define RFID_SS   5
#define RFID_SCK  18
#define RFID_MISO 19
#define RFID_MOSI 23
#define RFID_RST  27

hd44780_I2Cexp lcd;
MFRC522 rfid(RFID_SS, RFID_RST);

bool lcdReady = false;
bool rfidReady = false;
unsigned long lastScan = 0;
unsigned long lastCheck = 0;

void showMessage(const String &line1,
                 const String &line2) {
  Serial.println(line1 + " | " + line2);

  if (!lcdReady) return;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(line2.substring(0, 16));
}

String getCardUID() {
  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }
    uid += String(rfid.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();
  return uid;
}

void checkReader() {
  byte version = rfid.PCD_ReadRegister(
    MFRC522::VersionReg
  );

  if (version == 0x00 || version == 0xFF) {
    rfidReady = false;
    showMessage("RFID ERROR",
                "CHECK WIRING");
  } else {
    rfidReady = true;
    showMessage("RFID READY",
                "SCAN YOUR CARD");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Initialize LCD
  Wire.begin(LCD_SDA, LCD_SCL);
  Wire.setClock(100000);

  int status = lcd.begin(16, 2);

  if (status == 0) {
    lcdReady = true;
    lcd.backlight();
    showMessage("PROJECT 03",
                "RFID SYSTEM");
  } else {
    Serial.print("LCD error: ");
    Serial.println(status);
  }

  delay(1200);

  // Initialize RC522 SPI
  SPI.begin(
    RFID_SCK,
    RFID_MISO,
    RFID_MOSI,
    RFID_SS
  );

  pinMode(RFID_SS, OUTPUT);
  digitalWrite(RFID_SS, HIGH);

  rfid.PCD_Init();
  delay(100);

  checkReader();
}

void loop() {
  // Retry reader initialization if necessary
  if (!rfidReady) {
    if (millis() - lastCheck >= 3000) {
      lastCheck = millis();
      rfid.PCD_Init();
      delay(50);
      checkReader();
    }
    return;
  }

  // Return to waiting screen
  if (lastScan != 0 &&
      millis() - lastScan >= 3500) {
    lastScan = 0;
    showMessage("SCAN RFID CARD",
                "SYSTEM READY");
  }

  // Check for a new card
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Read card serial number
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = getCardUID();

  Serial.print("Detected UID: ");
  Serial.println(uid);

  if (uid.length() <= 16) {
    showMessage("CARD DETECTED", uid);
  } else {
    showMessage(uid.substring(0, 16),
                uid.substring(16, 32));
  }

  lastScan = millis();

  // End card communication
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

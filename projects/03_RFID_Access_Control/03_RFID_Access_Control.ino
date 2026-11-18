
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

// ===================================
// PROJECT 03 - RFID ACCESS CONTROL
// VERSION 3 - PASSIVE BUZZER DEBUG
// ESP32 Arduino Core 3.0.7
// ===================================

// LCD I2C pins
#define LCD_SDA 21
#define LCD_SCL 22

// RC522 RFID SPI pins
#define RFID_SS   5
#define RFID_RST  27
#define RFID_SCK  18
#define RFID_MISO 19
#define RFID_MOSI 23

// Passive buzzer
#define BUZZER_PIN 25
#define BUZZER_FREQ 2000

hd44780_I2Cexp lcd;
MFRC522 rfid(RFID_SS, RFID_RST);

bool lcdOK = false;
bool rfidOK = false;
bool cardDisplayed = false;

unsigned long lastCardTime = 0;
unsigned long lastRetryTime = 0;

// ===================================
// PASSIVE BUZZER FUNCTIONS
// ===================================

void toneOn(int frequency) {
  ledcWriteTone(BUZZER_PIN, frequency);
}

void toneOff() {
  ledcWriteTone(BUZZER_PIN, 0);
}

void beep(int count, int duration = 120) {
  for (int i = 0; i < count; i++) {
    toneOn(BUZZER_FREQ);
    delay(duration);

    toneOff();
    delay(150);
  }
}

// ===================================
// LCD DISPLAY FUNCTION
// ===================================

void showLCD(String line1, String line2) {
  Serial.println(line1 + " | " + line2);

  if (!lcdOK) {
    return;
  }

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16));

  lcd.setCursor(0, 1);
  lcd.print(line2.substring(0, 16));
}

// ===================================
// RFID UID FUNCTION
// ===================================

String getUID() {
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

// ===================================
// RFID INITIALIZATION TEST
// ===================================

bool initializeRFID() {
  rfid.PCD_Init();
  delay(100);

  byte version = rfid.PCD_ReadRegister(
    MFRC522::VersionReg
  );

  Serial.print("RC522 version: 0x");
  Serial.println(version, HEX);

  if (version == 0x00 || version == 0xFF) {
    return false;
  }

  return true;
}

// ===================================
// SETUP
// ===================================

void setup() {
  // STEP 1: Initialize passive buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  bool pwmOK = ledcAttach(
    BUZZER_PIN,
    BUZZER_FREQ,
    8
  );

  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("====================");
  Serial.println("PROJECT 03 VERSION 3");
  Serial.println("RFID + LCD + BUZZER");
  Serial.println("====================");

  if (pwmOK) {
    // One beep: firmware has started
    beep(1);
  }

  // STEP 2: Initialize LCD
  Wire.begin(LCD_SDA, LCD_SCL);
  Wire.setClock(100000);

  int lcdStatus = lcd.begin(16, 2);

  if (lcdStatus == 0) {
    lcdOK = true;
    lcd.backlight();

    showLCD("PROJECT 03 V3",
            "LCD TEST OK");

    // Two beeps: LCD initialized
    beep(2);
  } else {
    Serial.print("LCD ERROR: ");
    Serial.println(lcdStatus);

    // Two long beeps: LCD failure
    beep(2, 500);
  }

  delay(1500);

  // STEP 3: Initialize SPI
  showLCD("INITIALIZING",
          "RFID READER");

  SPI.begin(
    RFID_SCK,
    RFID_MISO,
    RFID_MOSI,
    RFID_SS
  );

  pinMode(RFID_SS, OUTPUT);
  digitalWrite(RFID_SS, HIGH);

  delay(100);

  // STEP 4: Initialize RFID reader
  rfidOK = initializeRFID();

  if (rfidOK) {
    showLCD("RFID READY",
            "SCAN YOUR CARD");

    // Three beeps: RFID communication OK
    beep(3);
  } else {
    showLCD("RFID ERROR",
            "CHECK SPI");

    // One long beep: RFID error
    beep(1, 800);
  }
}

// ===================================
// MAIN LOOP
// ===================================

void loop() {
  // Retry RFID if not detected
  if (!rfidOK) {
    if (millis() - lastRetryTime >= 3000) {
      lastRetryTime = millis();

      rfidOK = initializeRFID();

      if (rfidOK) {
        showLCD("RFID READY",
                "SCAN YOUR CARD");

        beep(3);
      }
    }

    return;
  }

  // Return to standby after card scan
  if (cardDisplayed &&
      millis() - lastCardTime >= 3500) {
    cardDisplayed = false;

    showLCD("SCAN RFID CARD",
            "SYSTEM READY");
  }

  // Check for a new RFID card
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Read card serial number
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = getUID();

  Serial.print("CARD UID: ");
  Serial.println(uid);

  // Display card UID
  if (uid.length() <= 16) {
    showLCD("CARD DETECTED", uid);
  } else {
    showLCD(
      uid.substring(0, 16),
      uid.substring(16, 32)
    );
  }

  // Four beeps: card detected
  beep(4);

  lastCardTime = millis();
  cardDisplayed = true;

  // End RFID communication
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

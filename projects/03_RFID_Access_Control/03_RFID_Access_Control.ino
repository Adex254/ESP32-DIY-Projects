
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

// ========================================
// PROJECT 03 - RFID ACCESS CONTROL
// VERSION 4 - DIAGNOSTIC FIRMWARE
// ESP32 Arduino Core 3.0.7
// ========================================

// LCD1602 I2C
#define LCD_SDA 21
#define LCD_SCL 22

// RC522 RFID SPI
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

bool buzzerOK = false;
bool lcdOK = false;
bool rfidOK = false;
bool cardDisplayed = false;

unsigned long lastCardTime = 0;
unsigned long lastRetryTime = 0;

// ========================================
// PASSIVE BUZZER
// ========================================

void beep(int count, int duration = 120) {
  if (!buzzerOK) return;

  for (int i = 0; i < count; i++) {
    ledcWriteTone(BUZZER_PIN, BUZZER_FREQ);
    delay(duration);

    ledcWriteTone(BUZZER_PIN, 0);
    delay(150);
  }
}

// ========================================
// LCD DISPLAY
// ========================================

void showLCD(const String &line1,
             const String &line2) {
  Serial.println(line1 + " | " + line2);

  if (!lcdOK) return;

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16));

  lcd.setCursor(0, 1);
  lcd.print(line2.substring(0, 16));
}

// ========================================
// RFID UID
// ========================================

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

// ========================================
// RFID INITIALIZATION
// ========================================

bool initializeRFID() {
  rfid.PCD_Init();
  delay(100);

  byte version =
    rfid.PCD_ReadRegister(MFRC522::VersionReg);

  Serial.print("RC522 version: 0x");
  Serial.println(version, HEX);

  // Known genuine MFRC522 versions.
  // Some compatible clones report other values.
  if (version == 0x91 || version == 0x92) {
    return true;
  }

  // 0x00 and 0xFF normally indicate
  // missing or failed SPI communication.
  if (version == 0x00 || version == 0xFF) {
    return false;
  }

  // Accept other responding versions for
  // card detection testing.
  Serial.println("Unrecognized RFID version");
  return true;
}

// ========================================
// SETUP
// ========================================

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("======================");
  Serial.println("PROJECT 03 VERSION 4");
  Serial.println("RFID LCD BUZZER TEST");
  Serial.println("======================");

  // Stage 1: Passive buzzer
  buzzerOK = ledcAttach(
    BUZZER_PIN,
    BUZZER_FREQ,
    8
  );

  // One beep = firmware started
  beep(1);

  // Stage 2: LCD
  Wire.begin(LCD_SDA, LCD_SCL);
  Wire.setClock(100000);

  int lcdStatus = lcd.begin(16, 2);

  if (lcdStatus == 0) {
    lcdOK = true;
    lcd.backlight();

    showLCD("PROJECT 03 V4",
            "LCD TEST OK");

    // Two short beeps = LCD ready
    beep(2);
  } else {
    Serial.print("LCD error: ");
    Serial.println(lcdStatus);

    // Two long beeps = LCD error
    beep(2, 500);
  }

  delay(2000);

  // Stage 3: SPI initialization
  showLCD("INITIALIZING",
          "RFID READER");

  pinMode(RFID_SS, OUTPUT);
  digitalWrite(RFID_SS, HIGH);

  SPI.begin(
    RFID_SCK,
    RFID_MISO,
    RFID_MOSI,
    RFID_SS
  );

  delay(100);

  // Stage 4: RFID initialization
  rfidOK = initializeRFID();

  if (rfidOK) {
    showLCD("RFID READY",
            "SCAN YOUR CARD");

    // Three beeps = RFID responding
    beep(3);
  } else {
    showLCD("RFID ERROR",
            "CHECK SPI");

    // One long beep = RFID error
    beep(1, 800);
  }
}

// ========================================
// MAIN LOOP
// ========================================

void loop() {

  // Retry if RFID reader failed
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

  // Return to standby after 3.5 seconds
  if (cardDisplayed &&
      millis() - lastCardTime >= 3500) {

    cardDisplayed = false;

    showLCD("SCAN RFID CARD",
            "SYSTEM READY");
  }

  // Wait for RFID card
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Read card UID
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = getUID();

  Serial.print("CARD UID: ");
  Serial.println(uid);

  // Show UID on LCD
  if (uid.length() <= 16) {
    showLCD("CARD DETECTED", uid);
  } else {
    showLCD(
      uid.substring(0, 16),
      uid.substring(16, 32)
    );
  }

  // Four short beeps = card detected
  beep(4);

  lastCardTime = millis();
  cardDisplayed = true;

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

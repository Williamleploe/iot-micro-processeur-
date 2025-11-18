#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Fingerprint.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// Pins
#define FP_RX 16
#define FP_TX 17
#define I2C_SDA 21
#define I2C_SCL 22
#define LCD_ADDR 0x27
#define RFID_SS 5
#define RFID_RST 4
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define SERVO_PIN 14

LiquidCrystal_I2C lcd(LCD_ADDR,16,2);
HardwareSerial FingerSerial(2);
Adafruit_Fingerprint finger(&FingerSerial);
MFRC522 rfid(RFID_SS, RFID_RST);
Servo lockServo;

const unsigned int SERVO_OPEN_POS = 90;
const unsigned int SERVO_CLOSED_POS = 0;
const unsigned long DISPLAY_MS = 1500;

void openLock() {
  lockServo.write(SERVO_OPEN_POS);
  delay(800);
  lockServo.write(SERVO_CLOSED_POS);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA,I2C_SCL);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("System Test");
  lcd.setCursor(0,1);
  lcd.print("Ready...");
  delay(1000);

  SPI.begin(SPI_SCK,SPI_MISO,SPI_MOSI);
  rfid.PCD_Init();

  FingerSerial.begin(57600, SERIAL_8N1, FP_RX, FP_TX);
  finger.begin(57600);

  lockServo.attach(SERVO_PIN);
  lockServo.write(SERVO_CLOSED_POS);

  if(finger.verifyPassword()){
    Serial.println("Fingerprint sensor OK");
  }else{
    Serial.println("Fingerprint sensor NOK");
  }
}

void loop() {
  // ----- RFID -----
  if(rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()){
    String uidStr = "";
    for(byte i=0;i<rfid.uid.size;i++){
      if(rfid.uid.uidByte[i]<0x10) uidStr += "0";
      uidStr += String(rfid.uid.uidByte[i],HEX);
      if(i+1<rfid.uid.size) uidStr += ":";
    }
    uidStr.toUpperCase();
    Serial.print("RFID detected: ");
    Serial.println(uidStr);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("RFID detected");
    lcd.setCursor(0,1);
    lcd.print(uidStr);

    // Déclenchement test
    openLock();

    rfid.PICC_HaltA();
    delay(DISPLAY_MS);
  }

  // ----- Fingerprint -----
  int p = finger.getImage();
  if(p == FINGERPRINT_OK){
    Serial.println("Finger detected!");
    if(finger.image2Tz(1) == FINGERPRINT_OK){
      if(finger.fingerFastSearch() == FINGERPRINT_OK){
        Serial.print("Fingerprint ID: ");
        Serial.println(finger.fingerID);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Finger OK ID:");
        lcd.setCursor(0,1);
        lcd.print(finger.fingerID);
        openLock();
      }else{
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Finger not found");
      }
    }
  }

  delay(500);
}

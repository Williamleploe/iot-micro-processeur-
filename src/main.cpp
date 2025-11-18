#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Fingerprint.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Preferences.h>

// ----------------- Pins -----------------
#define FP_RX 16   // ESP32 RX2 ← TX du FPM383C
#define FP_TX 17   // ESP32 TX2 → RX du FPM383C
#define I2C_SDA 21
#define I2C_SCL 22
#define LCD_ADDR 0x27
#define RFID_SS 5
#define RFID_RST 4
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define SERVO_PIN 14

// ----------------- Modules -----------------
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
HardwareSerial FingerSerial(2);
Adafruit_Fingerprint finger(&FingerSerial);
MFRC522 rfid(RFID_SS, RFID_RST);
Servo lockServo;
Preferences prefs;

// ----------------- Constantes -----------------
const unsigned int SERVO_OPEN_POS = 90;
const unsigned int SERVO_CLOSED_POS = 0;
const unsigned long DISPLAY_MS = 1500;
const char *PREF_NS = "auth";

// ----------------- Helpers -----------------
void lcdPrintBoth(const char *l1, const char *l2) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(l1);
  lcd.setCursor(0,1);
  lcd.print(l2);
}

String uidToKey(const MFRC522::Uid &u) {
  String s = "";
  for (byte i = 0; i < u.size; i++) {
    if (u.uidByte[i] < 0x10) s += "0";
    s += String(u.uidByte[i], HEX);
  }
  s.toUpperCase();
  return s;
}

void openLock() {
  lockServo.write(SERVO_OPEN_POS);
  delay(800);
  lockServo.write(SERVO_CLOSED_POS);
}

// ----------------- Persistence utilities -----------------
// Stored as: userN_type ("rfid" or "fp"), userN_key, userN_name
// count under "count", next fingerprint template id under "next_fp_id"
uint16_t userCount() {
  return prefs.getUInt("count", 0);
}
void setUserCount(uint16_t v) {
  prefs.putUInt("count", v);
}

void addUserRecordRaw(const char *type, const char *key, const char *name) {
  uint16_t n = userCount();
  String base = "user" + String(n) + "_";
  prefs.putString((base + "type").c_str(), type);
  prefs.putString((base + "key").c_str(), key);
  prefs.putString((base + "name").c_str(), name);
  setUserCount(n + 1);
}

void updateUserNameAtIndex(uint16_t idx, const char *name) {
  String base = "user" + String(idx) + "_";
  prefs.putString((base + "name").c_str(), name);
}

String findUserByRFID(const String &key) {
  uint16_t n = userCount();
  for (uint16_t i = 0; i < n; ++i) {
    String base = "user" + String(i) + "_";
    String t = prefs.getString((base + "type").c_str(), "");
    if (t == "rfid") {
      String k = prefs.getString((base + "key").c_str(), "");
      if (k == key) return prefs.getString((base + "name").c_str(), "");
    }
  }
  return "";
}

String findUserByFP(uint16_t id) {
  uint16_t n = userCount();
  Serial.print("Recherche FP ID: ");
  Serial.println(id);
  for (uint16_t i = 0; i < n; ++i) {
    String base = "user" + String(i) + "_";
    String t = prefs.getString((base + "type").c_str(), "");
    if (t == "fp") {
      String k = prefs.getString((base + "key").c_str(), "");
      Serial.print("Comparaison index ");
      Serial.print(i);
      Serial.print(" key stored: ");
      Serial.println(k);
      if (k == String(id)) {
        String name = prefs.getString((base + "name").c_str(), "");
        Serial.print("Match trouvé index ");
        Serial.print(i);
        Serial.print(" name ");
        Serial.println(name);
        return name;
      }
    }
  }
  Serial.println("Aucun utilisateur FP trouvé");
  return "";
}

void listUsers() {
  uint16_t n = userCount();
  Serial.print("Total users: ");
  Serial.println(n);
  for (uint16_t i = 0; i < n; ++i) {
    String base = "user" + String(i) + "_";
    String t = prefs.getString((base + "type").c_str(), "");
    String k = prefs.getString((base + "key").c_str(), "");
    String name = prefs.getString((base + "name").c_str(), "");
    Serial.print(i); Serial.print(": ");
    Serial.print(t); Serial.print(" | ");
    Serial.print(k); Serial.print(" | ");
    Serial.println(name);
  }
}

void clearAllUsers() {
  uint16_t n = userCount();
  for (uint16_t i = 0; i < n; ++i) {
    String base = "user" + String(i) + "_";
    prefs.remove((base + "type").c_str());
    prefs.remove((base + "key").c_str());
    prefs.remove((base + "name").c_str());
  }
  prefs.putUInt("count", 0);
  prefs.putUInt("next_fp_id", 1);
}

// ----------------- Enrollment flows -----------------
String readLineSerial(unsigned long timeout = 30000) {
  unsigned long start = millis();
  String s = "";
  while (millis() - start < timeout) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\r') continue;
      if (c == '\n') {
        if (s.length()) return s;
      } else s += c;
    }
    yield();
  }
  return s;
}

void enrollRFID() {
  lcdPrintBoth("Enroll RFID", "Scan card...");
  Serial.println(F("ENROLL RFID: Present card now"));
  unsigned long start = millis();
  while (millis() - start < 20000) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String uid = uidToKey(rfid.uid);
      Serial.print("Card UID: ");
      Serial.println(uid);
      lcdPrintBoth("Card detected:", uid.c_str());
      Serial.println("Type a name for this card and press Enter:");
      String name = readLineSerial();
      if (name.length() == 0) {
        Serial.println("Name timeout, abort.");
        lcdPrintBoth("Enroll aborted", "");
        delay(1500);
        rfid.PICC_HaltA();
        return;
      }
      addUserRecordRaw("rfid", uid.c_str(), name.c_str());
      Serial.print("RFID enrolled: ");
      Serial.println(name);
      lcdPrintBoth("RFID enrolled:", name.c_str());
      delay(1500);
      rfid.PICC_HaltA();
      return;
    }
    delay(100);
  }
  lcdPrintBoth("Enroll RFID", "Timeout");
  Serial.println("ENROLL RFID: Timeout");
}

// Improved fingerprint enrollment: store ID immediately, then ask name
bool enrollFingerprint() {
  lcdPrintBoth("Enroll Finger", "Place finger...");
  Serial.println(F("ENROLL FINGER: Follow prompts"));
  delay(300);

  int p;
  // Wait first image
  unsigned long start = millis();
  while (millis() - start < 20000) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) break;
    else if (p == FINGERPRINT_NOFINGER) {}
    else { Serial.print("getImage err1: "); Serial.println(p); }
    delay(200);
  }
  if (p != FINGERPRINT_OK) { lcdPrintBoth("Enroll failed", "no finger 1"); return false; }
  if (finger.image2Tz(1) != FINGERPRINT_OK) { lcdPrintBoth("Enroll failed", "img2tz1"); return false; }
  lcdPrintBoth("Remove finger", "");
  Serial.println("Remove finger");
  delay(1200);

  // Second image
  lcdPrintBoth("Place same finger", "again...");
  start = millis();
  while (millis() - start < 20000) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) break;
    delay(200);
  }
  if (p != FINGERPRINT_OK) { lcdPrintBoth("Enroll failed", "no finger 2"); return false; }
  if (finger.image2Tz(2) != FINGERPRINT_OK) { lcdPrintBoth("Enroll failed", "img2tz2"); return false; }

  if (finger.createModel() != FINGERPRINT_OK) { lcdPrintBoth("Enroll failed", "createModel"); return false; }

  // Determine ID and attempt to store
  uint16_t nextId = prefs.getUInt("next_fp_id", 1);
  uint16_t storedId = 0;
  for (uint16_t trial = 0; trial < 200; ++trial) {
    uint16_t id = nextId + trial;
    Serial.print("Tentative stockage ID ");
    Serial.println(id);
    int res = finger.storeModel(id);
    Serial.print("storeModel res = ");
    Serial.println(res);
    if (res == FINGERPRINT_OK) {
      storedId = id;
      Serial.print("Stored at ID ");
      Serial.println(id);
      prefs.putUInt("next_fp_id", id + 1); // advance next id
      break;
    } else if (res == FINGERPRINT_BADLOCATION) {
      Serial.println("Erreur: emplacement occupé, test suivant");
    } else if (res == FINGERPRINT_FLASHERR) {
      Serial.println("Erreur: écriture flash");
      lcdPrintBoth("Enroll failed", "flash err");
      return false;
    } else if (res == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Erreur: reception paquet");
      lcdPrintBoth("Enroll failed", "packet err");
      return false;
    } else {
      Serial.println("Erreur: code inconnu");
    }
    delay(200);
  }

  if (storedId == 0) { lcdPrintBoth("Enroll failed", "no slot"); return false; }

  // Store metadata placeholder, commit record immediately with placeholder name then update name
  uint16_t indexBefore = userCount();
  addUserRecordRaw("fp", String(storedId).c_str(), ("FP_" + String(storedId)).c_str());
  lcdPrintBoth("Enrolled ID:", String(storedId).c_str());
  Serial.print("Stored template ID: ");
  Serial.println(storedId);

  // Ask name (timeout allowed)
  Serial.println("Type a name for this fingerprint and press Enter (10s):");
  String name = readLineSerial(10000);
  if (name.length() == 0) {
    name = "FP_user_" + String(storedId);
    Serial.println("No name provided, using default: " + name);
  }
  // Update the name we just stored (indexBefore)
  updateUserNameAtIndex(indexBefore, name.c_str());
  Serial.print("Fingerprint enrolled: ");
  Serial.println(name);
  lcdPrintBoth("FP enrolled:", name.c_str());
  delay(1500);
  return true;
}

// ----------------- Utility commands -----------------
void showHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F(" r      -> enroll RFID"));
  Serial.println(F(" f      -> enroll Finger"));
  Serial.println(F(" list   -> list users"));
  Serial.println(F(" clear  -> clear all users (prefs)"));
  Serial.println(F(" delmod -> empty fingerprint database"));
  Serial.println(F(" help   -> show commands"));
}

void emptyFingerprintLibrary() {
  Serial.println("Attempting to empty fingerprint library...");
  int res = finger.emptyDatabase();
  Serial.print("emptyDatabase res = ");
  Serial.println(res);
  if (res == FINGERPRINT_OK) {
    Serial.println("Fingerprint DB emptied");
    lcdPrintBoth("FP DB", "emptied");
  } else {
    Serial.println("Failed to empty DB");
    lcdPrintBoth("FP DB", "erase failed");
  }
  delay(1500);
}

// ----------------- Setup / Loop -----------------
void setup() {
  Serial.begin(115200);
  delay(100);

  // Preferences
  prefs.begin(PREF_NS, false);

  // LCD
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcdPrintBoth("System starting", "");
  delay(800);

  // SPI RFID
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  rfid.PCD_Init();

  // Finger UART2
  FingerSerial.begin(57600, SERIAL_8N1, FP_RX, FP_TX);
  finger.begin(57600);

  // Servo
  lockServo.attach(SERVO_PIN);
  lockServo.write(SERVO_CLOSED_POS);

  // Startup messages and diagnostics
  bool fpok = finger.verifyPassword();
  if (fpok) {
    Serial.println("Fingerprint sensor OK");
  } else {
    Serial.println("Fingerprint sensor NOK");
  }

  Serial.println();
  Serial.println(F("\n--- AUTH SYSTEM READY ---"));
  showHelp();
  Serial.print("next_fp_id initial: ");
  Serial.println(prefs.getUInt("next_fp_id", 1));
  Serial.print("Stored users count: ");
  Serial.println(userCount());

  lcdPrintBoth("Ready", "Scan...");
  delay(500);
  lcd.setCursor(0,1);
  lcd.print("Scan...");
}

void loop() {
  // Serial commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "r") {
      enrollRFID();
      lcdPrintBoth("Ready", "Scan...");
    } else if (cmd == "f") {
      lcdPrintBoth("Enroll FP", "Follow serial");
      enrollFingerprint();
      lcdPrintBoth("Ready", "Scan...");
    } else if (cmd == "help") {
      showHelp();
    } else if (cmd == "list") {
      listUsers();
    } else if (cmd == "clear") {
      Serial.println("Clearing all user prefs...");
      clearAllUsers();
      Serial.println("Cleared");
    } else if (cmd == "delmod") {
      emptyFingerprintLibrary();
    } else {
      Serial.println("Unknown command. Type help");
    }
  }

  // RFID check
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String uid = uidToKey(rfid.uid);
    Serial.print("RFID detected: ");
    Serial.println(uid);
    String name = findUserByRFID(uid);
    if (name.length()) {
      Serial.print("Access granted: ");
      Serial.println(name);
      lcdPrintBoth("Access granted", name.c_str());
      openLock();
    } else {
      Serial.print("Access denied UID: ");
      Serial.println(uid);
      lcdPrintBoth("Access denied", uid.c_str());
    }
    rfid.PICC_HaltA();
    delay(DISPLAY_MS);
    lcdPrintBoth("Ready", "Scan...");
  }

  // Fingerprint check
  int p = finger.getImage();
  if (p == FINGERPRINT_OK) {
    if (finger.image2Tz(1) == FINGERPRINT_OK) {
      int res = finger.fingerFastSearch();
      if (res == FINGERPRINT_OK) {
        uint16_t id = finger.fingerID;
        Serial.print("Fingerprint found ID: ");
        Serial.println(id);
        String name = findUserByFP(id);
        if (name.length()) {
          Serial.print("Access granted: ");
          Serial.println(name);
          lcdPrintBoth("Access granted", name.c_str());
          openLock();
        } else {
          Serial.print("Access denied FP ID ");
          Serial.println(id);
          lcdPrintBoth("Access denied FP", String(id).c_str());
        }
        delay(DISPLAY_MS);
        lcdPrintBoth("Ready", "Scan...");
      } else {
        Serial.print("Fingerprint not found, search res = ");
        Serial.println(res);
      }
    } else {
      Serial.println("img2tz failed");
    }
  }
  delay(200);
}

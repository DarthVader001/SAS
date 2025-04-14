#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Fingerprint.h>
#include <heltec_unofficial.h>
#include <Keypad.h>
#include <EEPROM.h>
#define EEPROM_SIZE 512  // Define the EEPROM size

// --- RFID Pins ---
#define SS_PIN 21
#define RST_PIN 17
#define SCK_PIN 7
#define MOSI_PIN 6
#define MISO_PIN 5

// --- Fingerprint Pins ---
#define FINGER_TX 38
#define FINGER_RX 39

// --- WiFi ---
const char* ssid = "sooorabh";
const char* password = "anita1978";
const char* scriptURL = "https://script.google.com/macros/s/AKfycbzCtk91sscnohwqyeBfN1CrDgm9Dbvsv6dknA2TmbyXLpipQygMB_kiq-xNw8nrVITSCg/exec";

// --- Keypad ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {40, 41, 42, 45};
byte colPins[COLS] = {4, 3, 2, 1};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- User Pair Struct ---
struct UserPair {
  String rfid;
  int fingerprintID;
};

#define MAX_USERS 20
UserPair users[MAX_USERS];
int userCount = 0;

// --- Modules ---
MFRC522 mfrc522(SS_PIN, RST_PIN);
HardwareSerial mySerial(1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// --- Function Prototypes ---
void twoFactorAuth();
void registerFingerprint();
void deleteFingerprintID();
void sendData(String type, String data);

// --- EEPROM Functions ---
void saveUserData() {
    EEPROM.write(0, userCount);
    int addr = 1;
    for (int i = 0; i < userCount; i++) {
        EEPROM.put(addr, users[i]);
        addr += sizeof(UserPair);
    }
    EEPROM.commit();
}

void loadUserData() {
    userCount = EEPROM.read(0);
    if (userCount > MAX_USERS) userCount = 0;
    int addr = 1;
    for (int i = 0; i < userCount; i++) {
        EEPROM.get(addr, users[i]);
        addr += sizeof(UserPair);
    }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, FINGER_RX, FINGER_TX);
  finger.begin(57600);

  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  mfrc522.PCD_Init();

   // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    loadUserData();  // Load saved users from EEPROM

  heltec_setup();
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.drawString(0, 0, "Connecting to WiFi...");
  display.display();
  Serial.println("Connecting to WiFi...");

  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }

  display.clear();
  if (WiFi.status() == WL_CONNECTED) {
    display.drawString(0, 0, "WiFi Connected!");
    display.drawString(0, 12, "IP: " + WiFi.localIP().toString());
    Serial.println("WiFi Connected!");
  } else {
    display.drawString(0, 0, "WiFi Failed!");
    Serial.println("WiFi Failed!");
  }
  display.display();
  delay(2000);
}

// --- Loop ---
void loop() {
  display.clear();
  display.drawString(0, 0, "Two-Factor Mode");
  display.drawString(0, 12, "1. Authenticate");
  display.drawString(0, 24, "2. Register FP");
  display.drawString(0, 36, "3. Delete FP");
  display.display();

  Serial.println("Select Mode:");
  Serial.println("1. Authenticate");
  Serial.println("2. Register FP");
  Serial.println("3. Delete FP");

  char choice = 0;
  while (!choice) {
    char key = keypad.getKey();
    if (key == '1' || key == '2' || key == '3') {
      choice = key;
    }
    delay(100);
  }

  if (choice == '1') {
    twoFactorAuth();
  } else if (choice == '2') {
    registerFingerprint();
  } else if (choice == '3') {
    deleteFingerprintID();
  }
}

// --- Two-Factor Authentication ---
void twoFactorAuth() {

  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.drawString(0, 0, "Scan RFID...");
  Serial.println("Scan RFID...");
  display.display();  // Ensure this displays correctly

  // Add a delay to allow the display to show the message
  delay(1000);
  // Serial.println("Scan RFID...");

  String rfidID = "";
  unsigned long startTime = millis();
  const unsigned long timeout = 10000;

  // display.clear();
  // display.drawString(0, 0, "Waiting for RFID...");
  // display.display();

  // Serial.println("Waiting for RFID...");
  display.init();  // Reinitialize the display
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.drawString(0, 0, "Waiting for RFID...");
  display.display(); // Ensure the display updates

  Serial.println("Waiting for RFID...");

  // Allow a brief delay to ensure the message is visible
  delay(5000);

  while (millis() - startTime < timeout) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        rfidID += String(mfrc522.uid.uidByte[i], HEX);
      }
      rfidID.toUpperCase();
      Serial.println("RFID UID: " + rfidID);
      mfrc522.PICC_HaltA();
      delay(500);
      display.init();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.clear();
      display.drawString(0, 0, "RFID UID:");
      display.drawString(0, 12, rfidID);
      display.display();
      delay(1500);
       break;
    }
    delay(100);
  }
///////////////////////////////////
  if (rfidID == "") {
    delay(500);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.drawString(0, 0, "RFID Timeout!");
  display.display();
  Serial.println("RFID scan timed out.");
  delay(1500);

    return;
  }

  // Proceed to fingerprint scan
  delay(500);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.drawString(0, 0, "Scan Fingerprint...");
  display.display();
  Serial.println("Waiting for fingerprint...");
  delay(1500);

  int fingerprintID = -1;
  while (true) {
    int p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      delay(100);
      continue;
    } else if (p != FINGERPRINT_OK) {
      Serial.println("Image capture failed.");
       delay(500);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.clear();
      display.drawString(0, 0, "Capture Failed!");
      display.display();
      delay(1500);
      return;
    }

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
      Serial.println("Image convert failed.");
       delay(500);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.clear();
      display.drawString(0, 0, "Convert Failed!");
      display.display();
      delay(1500);
      return;
    }

    p = finger.fingerSearch();
    if (p != FINGERPRINT_OK) {
      Serial.println("No match found.");
       delay(500);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.clear();
      display.drawString(0, 0, "FP Not Found!");
      display.display();
      delay(1500);
      return;
    }

    fingerprintID = finger.fingerID;
    Serial.println("FP ID: " + String(fingerprintID));

    // --- RFID-Fingerprint Validation ---
    bool validPair = false;
    for (int i = 0; i < userCount; i++) {
      if (users[i].rfid == rfidID && users[i].fingerprintID == fingerprintID) {
        validPair = true;
        break;
      }
    }

    if (!validPair) {
       delay(500);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.clear();
      display.drawString(0, 0, "Mismatch Detected!");
      display.display();
      Serial.println("RFID and FP do not match!");
      delay(2000);
      return;
    }

    break;  // This stays after the validation
  }

  // Success
   delay(500);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.drawString(0, 0, "Access Granted!");
  display.drawString(0, 12, "RFID: " + rfidID);
  display.drawString(0, 24, "FP ID: " + String(fingerprintID));
  display.display();
  delay(2000);

  // Send to cloud
  sendData("RFID+and+FP", "RFID:" + rfidID + "|FP:" + String(fingerprintID));
}

// --- Fingerprint Registration ---
void registerFingerprint() {
  display.clear();
  display.drawString(0, 0, "Enter ID (1-127):");
  display.display();
  Serial.println("Enter ID (1-127):");

  String input = "";
  while (true) {
    char key = keypad.getKey();
    if (key && isDigit(key)) {
      input += key;
      display.clear();
      display.drawString(0, 0, "ID: " + input);
      display.display();
    } else if (key == '#') {
      break;
    }
    delay(100);
  }

  int id = input.toInt();
  if (id < 1 || id > 127) {
    display.clear();
    display.drawString(0, 0, "Invalid ID!");
    display.display();
    delay(1500);
    return;
  }

  // Scan RFID before registering fingerprint
  display.clear();
  display.drawString(0, 0, "Scan RFID to link...");
  display.display();
  Serial.println("Scan RFID to link...");
  delay(2000); // Reduce long delays

  String rfidID = "";
  unsigned long startTime = millis();
  const unsigned long timeout = 10000;
  while (millis() - startTime < timeout) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        rfidID += String(mfrc522.uid.uidByte[i], HEX);
      }
      rfidID.toUpperCase();
      mfrc522.PICC_HaltA();
      break;
    }
    delay(100);
  }

  if (rfidID == "") {
    display.init();  // Ensure display is initialized
    display.clear();
    display.drawString(0, 0, "RFID Timeout!");
    display.display();
    Serial.println("RFID Timeout");
    delay(1500);
    return;
  }

  // Keep the display active
  display.init();  // Ensure display stays initialized
  display.clear();
  display.drawString(0, 0, "Place finger...");
  display.display();
  Serial.println("Place finger...");

  delay(500);  // Short delay to prevent display flicker

  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz(1);

  display.clear();
  display.drawString(0, 0, "Remove finger...");
  display.display();
  delay(1500);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  display.clear();
  display.drawString(0, 0, "Place same finger...");
  display.display();
  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz(2);

  if (finger.createModel() == FINGERPRINT_OK && finger.storeModel(id) == FINGERPRINT_OK) {
    if (userCount < MAX_USERS) {
      users[userCount].rfid = rfidID;
      users[userCount].fingerprintID = id;
      userCount++;
      saveUserData();

      display.clear();
      display.drawString(0, 0, "Registered!");
      display.drawString(0, 12, "RFID: " + rfidID);
      display.drawString(0, 24, "FP ID: " + String(id));
      display.display();
      Serial.println("Registered RFID " + rfidID + " with FP ID " + String(id));
    } else {
      display.clear();
      display.drawString(0, 0, "User limit reached!");
      display.display();
    }
    delay(2000);
  } else {
    display.clear();
    display.drawString(0, 0, "Registration Failed!");
    display.display();
    delay(1500);
  }
}


// --- Send Data to Cloud ---
void sendData(String type, String data) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    return;
  }

  HTTPClient http;
  String url = String(scriptURL) + "?type=" + type + "&data=" + data;
  Serial.println("Sending data: " + url);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.println("HTTP error.");
  }

  http.end();
}

// --- Delete Fingerprint ---
void deleteFingerprintID() {
  const String adminPassword = "A123";  // Set your admin password here

  display.clear();
  display.drawString(0, 0, "Enter Password:");
  display.display();
  Serial.println("Enter Password:");

  String inputPassword = "";
  while (true) {
    char key = keypad.getKey();
    if (key && key != '#') {
      inputPassword += key;
      display.drawString(0, 12, "Password: " + inputPassword);
      display.display();
    } else if (key == '#') {
      break;  // Exit the loop when '#' is pressed
    }
    delay(100);
  }

  // Check if the entered password matches
  if (inputPassword != adminPassword) {
    display.clear();
    display.drawString(0, 0, "Wrong Password!");
    display.display();
    Serial.println("Wrong Password!");
    delay(2000);
    return;
  }

  // Proceed to enter ID to delete
  display.clear();
  display.drawString(0, 0, "Enter ID to delete:");
  display.display();
  Serial.println("Enter ID to delete:");

  String inputID = "";
  while (true) {
    char key = keypad.getKey();
    if (key && isDigit(key)) {
      inputID += key;
      display.drawString(0, 12, "ID: " + inputID);
      display.display();
    } else if (key == '#') {
      break;
    }
    delay(100);
  }

  int id = inputID.toInt();
  if (id < 1 || id > 127) {
    display.clear();
    display.drawString(0, 0, "Invalid ID!");
    display.display();
    delay(1500);
    return;
  }

  if (finger.deleteModel(id) == FINGERPRINT_OK) {
    display.clear();
    display.drawString(0, 0, "ID " + String(id) + " Deleted!");
    Serial.println("Fingerprint ID " + String(id) + " deleted.");
  } else {
    display.clear();
    display.drawString(0, 0, "Delete Failed!");
    Serial.println("Failed to delete ID " + String(id));
  }

  display.display();
  delay(2000);

  // Optional: remove the ID from local user array
  for (int i = 0; i < userCount; i++) {
    if (users[i].fingerprintID == id) {
      for (int j = i; j < userCount - 1; j++) {
        users[j] = users[j + 1];
      }
      userCount--;
      saveUserData();
      break;
    }
  }
}

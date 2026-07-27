#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define RST_PIN         9          
#define SS_PIN          10         
#define TRIG_PIN        2
#define ECHO_PIN        3
#define GREEN_LED       7
#define RED_LED         8
#define DRIVEWAY_LIGHT  A4        // White LED shared on I2C line cleanly
#define LDR_PIN         A5        // LDR Analog Input

const byte ROWS = 4; 
const byte COLS = 4; 
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {A0, A1, A2, A3}; 
byte colPins[COLS] = {0, 1, 4, 5}; 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

MFRC522 rfid(SS_PIN, RST_PIN);   
Servo gateServo;
LiquidCrystal_I2C lcd(0x27, 16, 2); 

byte ownerUID[4] = {0x11, 0x22, 0x33, 0x44}; 
const String secretPIN = "1234";
String inputPIN = "";

void setup() {
  SPI.begin();           
  rfid.PCD_Init();       
  gateServo.attach(6);  
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(DRIVEWAY_LIGHT, OUTPUT);

  lcd.init();
  lcd.backlight();
  resetSystemView();
}

void loop() {
  long distance = getDistance(TRIG_PIN, ECHO_PIN);
  char customKey = customKeypad.getKey();
  
  int lightLevel = analogRead(LDR_PIN);
  bool isDark = (lightLevel > 600); 

  if (distance < 50) { 
    lcd.setCursor(0, 0);
    lcd.print("Car At Gate!    ");
    
    if (isDark) {
      digitalWrite(DRIVEWAY_LIGHT, HIGH);
    }

    if (customKey) {
      if (customKey == '#') { 
        if (inputPIN == secretPIN) {
          handleAccessGranted("VIP Guest");
        } else {
          handleAccessDenied();
        }
        inputPIN = ""; 
      } else if (customKey == '*') { 
        inputPIN = "";
        lcd.setCursor(0, 1);
        lcd.print("Cleared!        ");
        delay(1000);
      } else {
        inputPIN += customKey;
        lcd.setCursor(0, 1);
        lcd.print("PIN: ");
        for(int i=0; i<inputPIN.length(); i++) lcd.print("*");
        lcd.print("                ");
      }
    } else if (inputPIN.length() == 0) {
      lcd.setCursor(0, 1);
      lcd.print("Scan RFID / PIN ");
    }
    
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      if (isOwnerCard()) {
        handleAccessGranted("Owner");
      } else {
        handleAccessDenied();
      }
      rfid.PICC_HaltA(); 
    }
  } else {
    resetSystemView();
    digitalWrite(DRIVEWAY_LIGHT, LOW); 
    inputPIN = "";
  }
  delay(50); 
}

long getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH);
  return duration * 0.034 / 2;
}

bool isOwnerCard() {
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != ownerUID[i]) return false;
  }
  return true;
}

void handleAccessGranted(String userType) {
  lcd.clear();
  lcd.print("Access Granted!");
  lcd.setCursor(0, 1);
  lcd.print("Welcome " + userType);
  digitalWrite(GREEN_LED, HIGH);
  
  gateServo.write(90); 
  delay(5000);         
  
  lcd.clear();
  lcd.print("Closing Gate... ");
  gateServo.write(0); 
  digitalWrite(GREEN_LED, LOW);
}

void handleAccessDenied() {
  lcd.clear();
  lcd.print("INVALID ACCESS!");
  lcd.setCursor(0, 1);
  lcd.print("Gate Locked     ");
  digitalWrite(RED_LED, HIGH);
  delay(3000);
  digitalWrite(RED_LED, LOW);
}

void resetSystemView() {
  lcd.setCursor(0, 0);
  lcd.print("SECURE GATE SYS ");
  lcd.setCursor(0, 1);
  lcd.print("Status: Locked  ");
  gateServo.write(0); 
}
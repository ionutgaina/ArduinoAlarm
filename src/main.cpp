#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>
#include <Wire.h>
#include <utils.hpp>

// PIR Sensor
const int pirPin = 2;
int pirState = LOW;
int val = 0;

// AUDIO

const int audioPin = A1;

// LED

const int ledPin = A0;

// MICROSD

const int microSDPin = 10;
File passwordFile;
// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int ROW_NUM = 4;    // four rows
const int COLUMN_NUM = 4; // four columns

char keys[ROW_NUM][COLUMN_NUM] = {{'1', '2', '3', 'A'},
                                  {'4', '5', '6', 'B'},
                                  {'7', '8', '9', 'C'},
                                  {'*', '0', '#', 'D'}};

byte pin_rows[ROW_NUM] = {9, 8, 7,
                          6}; // connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {
    5, 4, 3}; // connect to the column pinouts of the keypad

Keypad keypad =
    Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// global variables
bool isInitialized = false;
char mykey = ' ';
String enteredPIN = "";
String new_pin = "";
String current_pin = "";
const String DEFAULT_PIN = "1234";
const int PIN_LENGTH = 4;
bool settingnew_pin = false;
bool error = false;

String read_pin() {
  passwordFile = SD.open("pass.txt", FILE_READ);
  if (passwordFile) {
    current_pin = passwordFile.readString();
    passwordFile.close();
    return current_pin;
  } else {
    Serial.println("Error opening pass.txt for reading");
    return DEFAULT_PIN;
  }
}

bool set_new_pin(String new_pin) {
  if (new_pin.length() == PIN_LENGTH) {
    current_pin = new_pin;
    passwordFile = SD.open("pass.txt", O_WRITE | O_CREAT | O_TRUNC);
    if (passwordFile) {
      passwordFile.print(current_pin);
      passwordFile.close();
      return true;
    } else {
      Serial.println("Error opening pass.txt for writing");
      return false;
    }
  } else {
    Serial.println("Invalid PIN length");
    return false;
  }
}

void beep() {
  tone(audioPin, 1000);
  delay(100);
  noTone(audioPin);
}

void setup() {
  /* LCD INIT */
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  /* KEYPAD INIT */

  Serial.begin(9600);

  /* PIR SENSOR INIT */
  pinMode(pirPin, INPUT);

  /* LED */
  pinMode(ledPin, OUTPUT);

  /* AUDIO */
  pinMode(audioPin, OUTPUT);
  digitalWrite(audioPin, LOW);

  /* MicroSD */
  pinMode(microSDPin, OUTPUT);
  if (!SD.begin(microSDPin)) {
    error = true;
    Serial.println("SD Card Initialization Failed");
  }

  /* Read the current PIN */
  current_pin = read_pin();

  if (current_pin.length() != PIN_LENGTH) {
    if (!set_new_pin(DEFAULT_PIN)) {
      error = true;
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Resetting PIN");
      delay(2000);
    }
  }

  /* END SETUP */
  if (error) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error");
    lcd.setCursor(0, 1);
    lcd.print("Check Serial");
    while (1)
      ;
  }
  lcd.clear();
}

void loop() {}
// https://tronixstuff.com/2013/12/16/arduino-tutorials-chapter-42-numeric-keypads/
// https://lastminuteengineers.com/pir-sensor-arduino-tutorial/
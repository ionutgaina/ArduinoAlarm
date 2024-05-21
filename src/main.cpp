#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>
#include <Wire.h>
#include <utils.hpp>

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
bool error = false;

char mykey = ' ';
String entered_pin = "";
String current_pin = "";
File passwordFile;

STAGE stage = STAGE::DEACTIVATED;

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
  tone(AUDIO_PIN, 1000);
  delay(100);
  noTone(AUDIO_PIN);
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
  pinMode(PIR_PIN, INPUT);

  /* LED */
  pinMode(LED_PIN, OUTPUT);

  /* AUDIO */
  pinMode(AUDIO_PIN, OUTPUT);
  digitalWrite(AUDIO_PIN, LOW);

  /* MicroSD */
  pinMode(MICRO_SD_PIN, OUTPUT);
  if (!SD.begin(MICRO_SD_PIN)) {
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

void correctPIN() // do this if correct PIN entered
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("* Correct PIN *");
  delay(1000);
  lcd.clear();
  lcd.print("Enter PIN...");
}

void incorrectPIN() // do this if incorrect PIN entered
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" * Try again *");
  delay(1000);
  lcd.clear();
  lcd.print("Enter PIN...");
}

bool check_pin() {
  if (entered_pin == current_pin) {
    correctPIN();
    return true;
  } else {
    incorrectPIN();
    return false;
  }
}

void read_keypad() {
  char key = keypad.getKey();
  if (key) {
    beep();
    if (key == '#') {
      check_pin();
      entered_pin = "";
    } else if (key == '*') {
      entered_pin = "";
    } else {
      entered_pin += key;
    }
  }
  if (entered_pin.length() == PIN_LENGTH) {
    check_pin();
    entered_pin = "";
  }
}

void deactivated_stage() {
  // The alarm is not activated
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter PIN...");

  lcd.setCursor(0, 1);
  for (unsigned int i = 0; i < entered_pin.length(); i++) {
    lcd.print("*");
  }

  read_keypad();
}

void activated_stage() {
  // The alarm is activated
  lcd.print("Alarm Activated");
  delay(5000);
  stage = STAGE::DEACTIVATED;
}

void loop() {
  switch (stage) {
  case STAGE::DEACTIVATED:
    deactivated_stage();
    break;
  case STAGE::ACTIVATED:
    activated_stage();
    break;
  default:
    break;
  }
  delay(100);
}
// https://tronixstuff.com/2013/12/16/arduino-tutorials-chapter-42-numeric-keypads/
// https://lastminuteengineers.com/pir-sensor-arduino-tutorial/
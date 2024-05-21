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
  digitalWrite(LED_PIN, HIGH);

  /* AUDIO */
  pinMode(AUDIO_PIN, OUTPUT);
  digitalWrite(AUDIO_PIN, HIGH);

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

  digitalWrite(LED_PIN, HIGH);

  delay(1000);
  lcd.clear();
  lcd.print("Enter PIN...");
}

void incorrectPIN() // do this if incorrect PIN entered
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" * Try again *");

  digitalWrite(LED_PIN, LOW);

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

bool read_keypad() {
  char key = keypad.getKey();
  if (key || entered_pin.length() == PIN_LENGTH) {
    beep();
    if (key == '#' || entered_pin.length() == PIN_LENGTH) {
      if (entered_pin.length() == 0) {
        if (stage == STAGE::DEACTIVATED) {
          stage = STAGE::SETTINGS;
          return false;
        }
      }

      if (check_pin()) {
        entered_pin = "";
        return true;
      }
      entered_pin = "";

    } else if (key == '*') {
      entered_pin = "";
    } else {
      entered_pin += key;
    }
  }

  lcd.setCursor(0, 1);
  for (unsigned int i = 0; i < entered_pin.length(); i++) {
    lcd.print("*");
  }

  return false;
}

bool read_keypad_reset() {
  // change the PIN
  char key = keypad.getKey();
  if (key) {
    beep();
    if (key == '#') {
      if (entered_pin.length() == 0) {
        stage = STAGE::DEACTIVATED;
        return false;
      }

      if (!set_new_pin(entered_pin)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Err: len != " + String(PIN_LENGTH));
        delay(2000);
        entered_pin = "";
        return false;
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("PIN changed");
        delay(2000);
        entered_pin = "";
        return true;
      }
    } else if (key == '*') {
      entered_pin = "";
    } else {
      entered_pin += key;
    }
  }
  lcd.setCursor(0, 1);
  for (unsigned int i = 0; i < entered_pin.length(); i++) {
    lcd.print("*");
  }
  return false;
}

void deactivated_stage() {
  // The alarm is not activated
  digitalWrite(LED_PIN, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PIN to activate:");

  if (read_keypad()) {
    stage = STAGE::ACTIVATED;
  }
}

void activated_stage() {
  // The alarm is activated
  digitalWrite(LED_PIN, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PIN to deactivate:");
  if (read_keypad()) {
    stage = STAGE::DEACTIVATED;
  }
}

void settings_stage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter new PIN");

  if (read_keypad_reset()) {
    stage = STAGE::DEACTIVATED;
  }
}
void loop() {
  switch (stage) {
  case STAGE::SETTINGS:
    settings_stage();
    break;
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
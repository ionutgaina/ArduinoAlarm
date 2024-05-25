#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>
#include <TMRpcm.h>
#include <Wire.h>
#include "utils.hpp"

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad setup
const byte ROW_NUM = 4;
const byte COLUMN_NUM = 3;
char keys[ROW_NUM][COLUMN_NUM] = {
    {'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}};
byte pin_rows[ROW_NUM] = {8, 7, 6, 5};
byte pin_column[COLUMN_NUM] = {4, 3, 2};
Keypad keypad =
    Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// Global variables
bool error = false;
String entered_pin = "";
String current_pin = "";
TMRpcm tmrpcm;
STAGE stage = DEACTIVATED;

bool is_beeping = false;
unsigned long beep_start_time = 0;
unsigned long beep_duration = 100;
int beep_frequency = 0;

// Function prototypes
String read_pin();
bool set_new_pin(const String &new_pin);
void beep(int khz);
void correctPIN();
void incorrectPIN();
bool check_pin();
bool read_keypad();
bool read_keypad_reset();
void deactivated_stage();
void activated_stage();
void settings_stage();
void alarm_stage();
void interupt_handler();

void setup() {
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("Initializing..."));

  // Initialize Serial
  Serial.begin(9600);

  // PIR Sensor
  pinMode(PIR_PIN, INPUT);

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Initialize MicroSD
  pinMode(MICRO_SD_PIN, OUTPUT);
  if (!SD.begin(MICRO_SD_PIN)) {
    error = true;
    Serial.println(F("SD Card Initialization Failed"));
  }

  // audio
  tmrpcm.speakerPin = AUDIO_PIN;
  tmrpcm.quality(1);

  // Read the current PIN
  current_pin = read_pin();
  if (current_pin.length() != PIN_LENGTH && !set_new_pin(DEFAULT_PIN)) {
    error = true;
  } else if (current_pin.length() != PIN_LENGTH) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Resetting PIN"));
    delay(2000);
  }

  // Display error if any
  if (error) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Error"));
    lcd.setCursor(0, 1);
    lcd.print(F("Check Serial"));
    while (1) {
      delay(100);
    }
  }
  lcd.clear();
}

void loop() {
  switch (stage) {
  case SETTINGS:
    settings_stage();
    break;
  case DEACTIVATED:
    deactivated_stage();
    break;
  case ACTIVATED:
    activated_stage();
    break;
  case ALARM:
    alarm_stage();
    break;
  default:
    break;
  }

  if (is_beeping && (millis() - beep_start_time >= beep_duration)) {
    noTone(AUDIO_PIN);
    is_beeping = false;
  }

  delay(100);
}

String read_pin() {
  File passwordFile = SD.open("pass.txt", FILE_READ);
  if (passwordFile) {
    String pin = passwordFile.readString();
    passwordFile.close();
    return pin;
  } else {
    Serial.println(F("Error opening pass.txt for reading"));
    return DEFAULT_PIN;
  }
}

bool set_new_pin(const String &new_pin) {
  if (new_pin.length() == PIN_LENGTH) {
    current_pin = new_pin;
    File passwordFile = SD.open("pass.txt", FILE_WRITE);
    if (passwordFile) {
      passwordFile.print(current_pin);
      passwordFile.close();
      return true;
    } else {
      Serial.println(F("Error opening pass.txt for writing"));
      return false;
    }
  } else {
    Serial.println(F("Invalid PIN length"));
    return false;
  }
}

void beep(int khz) {
  beep_frequency = khz;
  is_beeping = true;
  beep_start_time = millis();
  tone(AUDIO_PIN, beep_frequency);
}

void correctPIN() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("* Correct PIN *"));
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  lcd.clear();
  lcd.print(F("Enter PIN..."));
}

void incorrectPIN() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F(" * Try again *"));
  digitalWrite(LED_PIN, LOW);
  delay(1000);
  lcd.clear();
  lcd.print(F("Enter PIN..."));
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
    beep(500);
    if (key == '#' || entered_pin.length() == PIN_LENGTH) {
      if (entered_pin.length() == 0 && stage == DEACTIVATED) {
        stage = SETTINGS;
        return false;
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
  char key = keypad.getKey();
  if (key) {
    beep(500);
    if (key == '#') {
      if (entered_pin.length() == 0) {
        stage = DEACTIVATED;
        return false;
      }
      if (!set_new_pin(entered_pin)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("Err: len != 4"));
        delay(2000);
        entered_pin = "";
        return false;
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("PIN changed"));
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
  digitalWrite(LED_PIN, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("PIN to activate:"));
  if (read_keypad()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Activating..."));
    noTone(AUDIO_PIN);
    delay(5000);
    stage = ACTIVATED;
  }
}

void activated_stage() {
  digitalWrite(LED_PIN, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("PIN to deactivate:"));
  if (read_keypad()) {
    stage = DEACTIVATED;
  }

  if (digitalRead(PIR_PIN) == HIGH) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("INTRUDER ALERT"));
    noTone(AUDIO_PIN);
    delay(5000);
    stage = ALARM;
  }
}

void settings_stage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Enter new PIN"));
  if (read_keypad_reset()) {
    stage = DEACTIVATED;
  }
}

bool sound = false;

void alarm_stage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("ALARM ACTIVATED"));

  sound = !sound;

  if (sound) {
    beep(1000);
  } else {
    beep(2000);
  }

  if (digitalRead(LED_PIN) == HIGH) {
    digitalWrite(LED_PIN, LOW);
  } else {
    digitalWrite(LED_PIN, HIGH);
  }

  if (read_keypad()) {
    stage = DEACTIVATED;
  }
}

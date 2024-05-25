#ifndef AF01E42E_D04B_428E_9046_3AC3100CE31A
#define AF01E42E_D04B_428E_9046_3AC3100CE31A

// Constants and Pin Definitions
const int PIR_PIN = A1;
const int AUDIO_PIN = 9;
const int LED_PIN = A0;
const int MICRO_SD_PIN = 10;
const String DEFAULT_PIN = "1234";
const int PIN_LENGTH = 4;

// Enum for stages
enum STAGE {
  DEACTIVATED,
  ACTIVATED,
  ALARM,
  SETTINGS
};

#endif /* AF01E42E_D04B_428E_9046_3AC3100CE31A */

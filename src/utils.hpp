#ifndef AF01E42E_D04B_428E_9046_3AC3100CE31A
#define AF01E42E_D04B_428E_9046_3AC3100CE31A
// PIR Sensor
const int PIR_PIN = 2;

// AUDIO
const int AUDIO_PIN = A0;

// LED
const int LED_PIN = A1;

// MicroSD
const int MICRO_SD_PIN = 10;

// PIN
const String DEFAULT_PIN = "1234";
const int PIN_LENGTH = 4;

enum STAGE
{
    DEACTIVATED,
    ACTIVATED,
    ALARM,
    SETTINGS
};

#endif /* AF01E42E_D04B_428E_9046_3AC3100CE31A */

#ifndef LED_FAN_DEFAULT_SETUP_H
#define LED_FAN_DEFAULT_SETUP_H

// ==================================================================
// ======================  Setup  ===================================
// ==================================================================

// ------------------------------------------
// ---- General
// ------------------------------------------

#define MAX_FRAMES_PER_SECOND 10000

// ------------------------------------------
// ---- Wifi
// ------------------------------------------

//#define WIFI_ENABLED
#define HOST_NETWORK_SSID "LLED Strip"
#define HOST_NETWORK_PASSWORD "We love LED"
#define WIFI_HOSTNAME "lled.wifi"

// ------------------------------------------
// ---- Screen
// ------------------------------------------

#define LED_DATA_PIN 13

// Only in use for clock-based LED strips (e.g. APA102)
#define LED_CLOCK_PIN 14
// Max 80, higher values need shorter wire length
#define LED_CLOCK_SPEED_MHZ 60

#define LED_COUNT 256
// How many additional LEDs we should set to black, just to be safe
#define LED_OVERFLOW_WALL 0

// A value of 0 to 255 where 255 is no rescale, and 1 is "full" rescale.
// Uses LED's "global brightness" parameter to dynamically recompute pixels
// into a scale where we have better resolution.
#define MAX_DYNAMIC_COLOR_RESCALE 8

// Natural, or rather "minimum" response of LEDs.
#define NATURAL_COLOR_RESPONSE 2.2f

// ------------------------------------------
// ---- FastLED
// ------------------------------------------

// Define to use FastLED. If Apa102 is used, don't define.
// See https://github.com/FastLED/FastLED/blob/master/chipsets.h
// APA102Controller recommended, WS2013 etc. work too
//#define FastLED_LED_TYPE APA102Controller
#define COLOR_ORDER BGR

// In bytes
// 2 words per LED, + some boundary + buffer
#define SPI_BUFFER_SIZE (4 * LED_COUNT + 100)

// Set to a valid SPI host to route all SPI outputs to this
#define SPI_ESP32_HARDWARE_SPI_HOST HSPI_HOST

// ------------------------------------------
// ---- Other
// ------------------------------------------

// If the power supply is limited, define this to dynamically limit drawn power
#define MAX_AMPERE 1
#define AMPERE_PER_LED 0.06

#define PAIR_PIN 27

#define LUT_SIN_COUNT 1000

#endif //LED_FAN_DEFAULT_SETUP_H

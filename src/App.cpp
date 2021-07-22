//
// Created by Lukas Tenbrink on 27.04.20.
//

#include "App.h"

#ifdef FastLED_LED_TYPE
#include "util/spi/ESP32SPI.h"
#include <screen/FastLEDRenderer.h>
#endif

#include <SPIFFS.h>
#include <network/Network.h>
#include <screen/Apa102Renderer.h>

#include <util/Logger.h>
#include <util/LUT.h>
#include <screen/behavior/Behaviors.h>

#define MICROSECONDS_PER_FRAME (1000 * 1000 / MAX_FRAMES_PER_SECOND)

App::App() {
    SerialLog.print("Booting LLED WiFi Firmware").ln();

    // Don't init Device ID until asked via HTTP
    // Because WiFi gives excellent entropy
//    DeviceID::get();

    // Mount file system
    SPIFFS.begin(false);

#ifdef WIFI_ENABLED
    // Initialize WiFi first, we want to connect ASAP
    Network::setHostname(WIFI_HOSTNAME);
    Network::setSoftApSetup(new WifiSetup(HOST_NETWORK_SSID, HOST_NETWORK_PASSWORD));
    Network::readConfig();
    // Start connecting
    Network::checkStatus();
#endif

    // Init Statics
    LUT::initSin(LUT_SIN_COUNT);
    NativeBehaviors::init();

    // Clock Synchronizer
    regularClock = new RegularClock(
        MICROSECONDS_PER_FRAME, 50
    );

    // Initialize Screen

#ifdef FastLED_LED_TYPE
    auto controller = new FastLED_LED_TYPE<LED_DATA_PIN, LED_CLOCK_PIN, COLOR_ORDER, DATA_RATE_MHZ(LED_CLOCK_SPEED_MHZ)>();
    auto renderer = new FastLEDRenderer(LED_COUNT, LED_OVERFLOW_WALL, controller);
#else
    auto renderer = new Apa102Renderer(LED_COUNT, LED_OVERFLOW_WALL);
    SerialLog.print(
        "Attaching Apa102 Renderer with "
        + String(renderer->pixelCount)
        + " + " + String(LED_OVERFLOW_WALL)
        + " pixels."
    ).ln();
#endif
    renderer->setColorCorrection(0xFFB0F0);
    renderer->setMaxDynamicColorRescale(MAX_DYNAMIC_COLOR_RESCALE);

    screen = new Screen(renderer);
    // Startup Animation
    screen->behavior = new Ping(2000 * 1000);

#ifdef MAX_AMPERE
    float peakAmpereDrawn = (LED_COUNT) * AMPERE_PER_LED;
    if (peakAmpereDrawn > MAX_AMPERE) {
        float allowedRatio = float(MAX_AMPERE) / peakAmpereDrawn;
        // Each LED has a lightness of 3 (r+g+b)
        screen->renderer->setMaxLightness(float((LED_COUNT) * 3) * allowedRatio);
    }
#endif

    pairPin = PAIR_PIN;
    pinMode(pairPin, INPUT_PULLUP);

#ifdef WIFI_ENABLED
    // Initialize Server
    artnetServer = new ArtnetServer(screen);
    updater = new Updater();

    server = new HttpServer(this);
#endif
}

void App::run() {
    auto delayMicros = regularClock->sync();

    screen->update(delayMicros);

    if (delayMicros > timeUntilSlowUpdate) {
        timeUntilSlowUpdate = 1000 * 1000 * 2;

#ifdef WIFI_ENABLED
        if (digitalRead(pairPin) == LOW) {
            Network::pair();
        }
#endif
    }
    else
        timeUntilSlowUpdate -= delayMicros;

#ifdef WIFI_ENABLED
    updater->handle();
#endif
}

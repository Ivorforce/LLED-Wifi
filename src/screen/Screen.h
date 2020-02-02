//
// Created by Lukas Tenbrink on 20.01.20.
//

#ifndef LED_FAN_SCREEN_H
#define LED_FAN_SCREEN_H

#include <FastLED.h>
#include <util/IntRoller.h>

class Screen {
public:
    enum Mode {
        demo, screen, concentric
    };

    int count;
    CRGB *leds;

    Mode mode = demo;

    unsigned long lastFrameTime = 0;

    int virtualSize;
    CRGB *virtualScreen;

    IntRoller *concentricResolution;
    CRGB *concentricScreen;

    unsigned long millisecondsPingLeft = 0;

    Screen(int ledCount, int virtualSize);

    void draw(unsigned long milliseconds, float rotation);

    void drawDemo(unsigned long milliseconds, float rotation);
    void drawScreen(unsigned long milliseconds, float rotation);
    void drawConcentric(unsigned long milliseconds, float rotation);
    void drawError();

    int pin();

    void drawRGB(float red, float green = 0, float blue = 0);

    int ping();
};


#endif //LED_FAN_SCREEN_H
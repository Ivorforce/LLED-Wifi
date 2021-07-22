//
// Created by Lukas Tenbrink on 20.01.20.
//

#ifndef LED_FAN_SCREEN_H
#define LED_FAN_SCREEN_H

static const int MICROS_INPUT_ACTIVE = 5000 * 1000;

#include <util/IntRoller.h>
#include <screen/behavior/NativeBehavior.h>
#include <util/Image.h>
#include "Renderer.h"

class Screen {
public:
    Renderer *renderer;

    unsigned long lastUpdateTimestamp;

    // Multi-purpose buffer for any input mode
    PRGB *buffer;
    int bufferSize;

    PRGB *pixels;

    NativeBehavior *behavior = nullptr;

    Screen(Renderer *renderer);

    void readConfig();

    void update(unsigned long delayMicros);

    void draw(unsigned long delayMicros);

    int getPixelCount() {
        return renderer->pixelCount;
    }

    float getBrightness() const {
        return renderer->getBrightness();
    };
    void setBrightness(float brightness);

    float getResponse() const;;
    void setResponse(float response);;

};


#endif //LED_FAN_SCREEN_H

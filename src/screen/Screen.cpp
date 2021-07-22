//
// Created by Lukas Tenbrink on 20.01.20.
//

#include "Screen.h"

#include <util/Logger.h>
#include <util/Image.h>

#include <cmath>
#include <util/Profiler.h>
#include <Setup.h>
#include <util/TextFiles.h>
#include <util/StringRep.h>
#include <numeric>
#include <esp32-hal.h>

#include "behavior/StrobeDemo.h"


// FIXME This should definitely be per-instance

Screen::Screen(Renderer *renderer)
: renderer(renderer), bufferSize(renderer->pixelCount) {
    buffer = new PRGB[bufferSize]{PRGB::black};
    pixels = renderer->rgb;

    readConfig();
}

void Screen::readConfig() {
    renderer->setBrightness(StringRep::toFloat(TextFiles::readConf("brightness"), 1.0f));
    setResponse(StringRep::toFloat(TextFiles::readConf("response"), 1));
}

void Screen::update(unsigned long delayMicros) {
    lastUpdateTimestamp = micros();
    draw(delayMicros);
}

void Screen::draw(unsigned long delayMicros) {
    if (behavior == nullptr) {
        // Nothing to show, let's switch back to Demo.
        behavior = new StrobeDemo();
    }

    if (behavior != nullptr) {
        auto status = behavior->update(this, delayMicros);

        if (status == NativeBehavior::alive || (status == NativeBehavior::purgatory)) {
            renderer->render();
            return;
        }

        // Behavior over, reset pointer
        behavior = nullptr;
    }

    memset((void *)renderer->rgb, 0, renderer->pixelCount * 3); // Fill black
    renderer->render();
}

void Screen::setBrightness(float brightness) {
    // Let's not go overboard with the brightness
    renderer->setBrightness(std::min(brightness, 255.0f));
    TextFiles::writeConf("brightness", String(brightness));
}

float Screen::getResponse() const {
    return renderer->getResponse() - float(NATURAL_COLOR_RESPONSE) + 1;
}

void Screen::setResponse(float response) {
    response = std::max(1.0f, std::min(10.0f, response));
    TextFiles::writeConf("response", String(response));

    response += float(NATURAL_COLOR_RESPONSE) - 1;
    renderer->setResponse(response);
}



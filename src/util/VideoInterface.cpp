//
// Created by Lukas Tenbrink on 21.01.20.
//

#include <Setup.h>
#include <screen/behavior/Behaviors.h>
#include "VideoInterface.h"
#include "DeviceID.h"
#include "Logger.h"

VideoInterface::VideoInterface(Screen *screen, ArtnetServer *artnetServer) : screen(screen),
                                                                             artnetServer(artnetServer) {
}

void VideoInterface::info(AsyncResponseStream *stream) {
    const std::vector<ArtnetEndpoint *> *endpoints = artnetServer->endpoints();
    auto behaviors = NativeBehaviors::list;

    const String deviceID = DeviceID::getString();

    DynamicJsonDocument doc(
            JSON_OBJECT_SIZE(4)
            // Device ID
            + deviceID.length()
            // Behaviors
            + JSON_ARRAY_SIZE(behaviors.size())
            // Cartesian Screen
            + JSON_OBJECT_SIZE(2)
            + 512 // Capacity for other strings
    );

    doc["id"] = deviceID.begin();

    {
        auto list = doc.createNestedArray("behaviors");
        for (auto name : behaviors.keys) {
            list.add(name.begin());
        }
    }

    // ==============================================
    // ==================Pixels======================
    // ==============================================

    {
        auto object = doc.createNestedObject("cartesian");

        object["net"] = (*endpoints)[0]->net;
        object["pixels"] = screen->getPixelCount();
    }

    serializeJsonPretty(doc, *stream);
}

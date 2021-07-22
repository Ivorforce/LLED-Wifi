//
// Created by Lukas Tenbrink on 10.07.20.
//

#include "Behaviors.h"

NativeBehaviors::Dict NativeBehaviors::list = NativeBehaviors::Dict();

void NativeBehaviors::init() {
    list.clear();

    list.put("None", []() {
        return nullptr;
    });
    list.put("Demo", []() {
        return new StrobeDemo();
    });
    list.put("StrobeDemo", []() {
        return new StrobeDemo();
    });
    list.put("Strobe", []() {
        return new StrobeDemo();
    });
    list.put("Dotted", []() {
        return new StrobeDemo();
    });
    list.put("Cartesian Demo", []() {
        return new StrobeDemo();
    });
}

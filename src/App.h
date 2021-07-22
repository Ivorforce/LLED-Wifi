//
// Created by Lukas Tenbrink on 27.04.20.
//

#ifndef LED_FAN_APP_H
#define LED_FAN_APP_H


#include <screen/Screen.h>
#include <network/HttpServer.h>
#include <network/ArtnetServer.h>
#include <util/RegularClock.h>
#include <network/Updater.h>

class App {
public:
    Screen *screen;

    HttpServer *server;
    ArtnetServer *artnetServer;

    Updater *updater;

    RegularClock *regularClock;

    App();

    void run();

    int pairPin;
    int timeUntilSlowUpdate = 1000;
};


#endif //LED_FAN_APP_H

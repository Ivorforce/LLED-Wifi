//
// Created by Lukas Tenbrink on 20.01.20.
//

#include <WiFi.h>
#include <SPIFFS.h>
#include <util/TextFiles.h>
#include <util/Logger.h>
#include "Network.h"

WifiMode Network::mode = WifiMode::accessPoint;
bool Network::needsReconnect = false;
WifiSetup *Network::stationSetup = new WifiSetup("", "");
WifiSetup *Network::softAPSetup = new WifiSetup("", "");

WifiSetup::WifiSetup(const String &ssid, const String &password) : ssid(ssid), password(password) {}

void WifiSetup::trim() {
    password.trim();
    ssid.trim();
}

void WifiSetup::write(String file) {
    TextFiles::writeConf(file, ssid + "\n" + password);
}

bool WifiSetup::read(String file) {
    String s = TextFiles::readConf(file);

    int newline = s.indexOf('\n');
    if (newline <= 0) {
        ssid = password = "";
        return false;
    }

    ssid = s.substring(0, newline);
    password = s.substring(newline + 1);

    return isComplete();
}

bool WifiSetup::isComplete() {
    return !ssid.isEmpty() && (password.isEmpty() || password.length() >= 8);
}

bool WifiSetup::operator==(const WifiSetup &rhs) const {
    return ssid == rhs.ssid &&
           password == rhs.password;
}

bool WifiSetup::operator!=(const WifiSetup &rhs) const {
    return !(rhs == *this);
}

void Network::setHostname(String hostname) {
    WiFi.setHostname(hostname.begin());
}

bool Network::checkStatus() {
    wl_status_t status = WiFi.status();

    switch (mode) {
        case WifiMode::station:
            // Don't call connect() again if status != connected
            // because it will cancel current connection attempts.

            if (needsReconnect || WiFi.getMode() != WIFI_MODE_STA || status == WL_CONNECT_FAILED)
                connectToStation(1);

            break;
        case WifiMode::accessPoint:
            if (needsReconnect || WiFi.getMode() != WIFI_MODE_AP)
                hostSoftAP();

            break;
        default:
            break;
    }

    return status == WL_CONNECTED;
}

bool Network::connectToStation(int tries) {
    if (tries < 0)
        return false;

    if (!stationSetup->isComplete()) {
        mode = WifiMode::accessPoint;
        SerialLog.print("Invalid Station Setup! Pairing!").ln();
        return false;
    }

    SerialLog.print("Connecting to " + stationSetup->ssid + "...").ln();

    needsReconnect = false;
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(stationSetup->ssid.begin(), stationSetup->passwordPtr());

    for (int i = tries; i > 0; --i) {
        if (WiFi.status() == WL_CONNECTED) {
            break;
        }
        
        if (i <= 1) {
            return false;
        }
        
        delay(1000);
    }

    return true;
}

IPAddress Network::address() {
    if (WiFi.getMode() == WIFI_MODE_STA)
        return WiFi.localIP();

    return WiFi.softAPIP();
}

void Network::pair() {
    SerialLog.print("Pairing???").ln();

    // Clear current pairing
    setStationSetup(new WifiSetup("", ""));
    mode = WifiMode::accessPoint;
}

void Network::readConfig() {
    if (stationSetup->read(STATION_SETUP_FILE)) {
        mode = WifiMode::station;
        connectToStation(1);
        return;
    }

    mode = WifiMode::accessPoint;
    hostSoftAP();
}

void Network::hostSoftAP() {
    if (!softAPSetup->isComplete()) {
        SerialLog.print("Failed to host access point!").ln();
        return;
    }

    needsReconnect = false;

    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAP(softAPSetup->ssid.begin(), softAPSetup->passwordPtr());
}

WifiSetup *Network::getSoftApSetup() {
    return softAPSetup;
}

void Network::setSoftApSetup(WifiSetup *softAPSetup) {
    softAPSetup->trim();

    if (mode == WifiMode::accessPoint && Network::softAPSetup != softAPSetup)
        needsReconnect = true;

    Network::softAPSetup = softAPSetup;
}

WifiSetup *Network::getStationSetup() {
    return stationSetup;
}

void Network::setStationSetup(WifiSetup *stationSetup) {
    stationSetup->trim();

    if (mode == WifiMode::station && Network::stationSetup != stationSetup)
        needsReconnect = true;

    Network::stationSetup = stationSetup;
    stationSetup->write(STATION_SETUP_FILE);
}

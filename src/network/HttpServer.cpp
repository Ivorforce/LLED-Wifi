//
// Created by Lukas Tenbrink on 20.01.20.
//

#include "HttpServer.h"
#include "Network.h"

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <esp_wifi.h>
#include <util/Logger.h>
#include <screen/behavior/Ping.h>
#include <App.h>
#include <util/Profiler.h>
#include <WiFi.h>
#include <screen/behavior/Behaviors.h>

#include <utility>
#include <util/CrudeJson.h>

#define SERVE_HTML(uri, file) _server.on(uri, HTTP_GET, [template_processor](AsyncWebServerRequest *request){\
    request->send(SPIFFS, file, "text/html", false, template_processor);\
});

#define request_result(success) request->send(success ? 200 : 400, "text/plain", success ? "Success" : "Failure"); return

using namespace std::placeholders;

// FIXME This should be per-instance. Or something.
AsyncWebServer _server(80);

HttpServer::HttpServer(App *app) : app(app), videoInterface(new VideoInterface(app->screen, app->artnetServer)) {
    setupRoutes();
    _server.begin();
}

String HttpServer::processTemplates(const String &var) {
    if (var == "AP_IP")
        return Network::address().toString();
    if (var == "AP_SSID") {
        wifi_config_t conf_current;
        esp_wifi_get_config(WIFI_IF_AP, &conf_current);
        return String(reinterpret_cast<char*>(conf_current.ap.ssid));;
    }

    if (var == "LOCAL_IP")
        return WiFi.localIP().toString();
    if (var == "LOCAL_SSID")
        return String(WiFi.SSID());

    if (var == "LED_PIN")
#if LED_TYPE == APA102Controller
        return String(LED_DATA_PIN) + ", Clock: " + String(LED_CLOCK_PIN);
#else
        return String(app->screen->pin);
#endif

    if (var == "UPTIME") {
        return Profiler::readableTime(esp_timer_get_time(), 2);
    }
    if (var == "FPS") {
        unsigned long meanMicrosPerFrame = app->regularClock->frameTimeHistory->mean();

        auto fpsString = String(1000 * 1000 / std::max(meanMicrosPerFrame, app->regularClock->microsecondsPerFrame))
            + " (slack: " + String(_max(0, (int) (app->regularClock->microsecondsPerFrame - meanMicrosPerFrame))) + "µs)";

        return fpsString;
    }

    return String("ERROR");
}

bool handlePartialFile(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!index) {
        request->_tempFile = SPIFFS.open("/img", FILE_WRITE);
    }
    request->_tempFile.write(data, len);

    if(index + len == total) {
        // Final Packet
        request->_tempFile.close();
        request->_tempFile = SPIFFS.open("/img", FILE_READ);

        return true;
    }

    return false;
}

void registerREST(const char* url, String param, const std::function<String(String)>& set, const std::function<String()>& get) {
    _server.on(url, HTTP_POST, [param, set](AsyncWebServerRequest *request) {
        if (!request->hasParam(param, true)) {
            request_result(false);
        }

        auto value = request->getParam(param, true)->value();
        auto result = set(value);
        if (result.isEmpty())
            request->send(400, "text/plain", "Failure");
        else
            request->send(200, "text/plain", result);
    });

    _server.on(url, HTTP_GET, [get](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", get());
    });
}

void HttpServer::setupRoutes() {
    auto template_processor = std::bind(&HttpServer::processTemplates, this, _1);
    auto videoInterface = this->videoInterface;
    auto screen = app->screen;
    auto updater = app->updater;

    _server.serveStatic("/material.min.js", SPIFFS, "/material.min.js");
    _server.serveStatic("/material.min.css", SPIFFS, "/material.min.css");

    _server.serveStatic("/images", SPIFFS, "/images");
    _server.serveStatic("/styles.css", SPIFFS, "/styles.css");
    _server.serveStatic("/scripts.js", SPIFFS, "/scripts.js");

    SERVE_HTML("/", "/index.html")

    // -----------------------------------------------
    // ---------------- Settings ---------------------
    // -----------------------------------------------

    SERVE_HTML("/settings", "/settings.html")

    _server.on("/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
            request_result(false);
        }

        auto ssid = request->getParam("ssid", true)->value();
        auto password = request->getParam("password", true)->value();

        Network::setStationSetup(new WifiSetup(ssid, password));
        Network::mode = WifiMode::station;

        // We may connect later, but it's deferred
        request_result(true);
    });

    _server.on("/wifi/pair", HTTP_POST, [](AsyncWebServerRequest *request) {
        Network::pair();
        request_result(true);
    });

    // -----------------------------------------------
    // ------------------ Other ----------------------
    // -----------------------------------------------

    _server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "404 / Not Found");
    });

    _server.on("/ping", HTTP_POST, [screen](AsyncWebServerRequest *request) {
        unsigned long time = 2000 * 1000;

        screen->behavior = new Ping(time);
        WifiLog.print("Pong").ln();
        request->send(200, "text/plain", String(time));
    });

    registerREST("/behavior", "id", [screen](String id) {
        auto provider = NativeBehaviors::list[std::move(id)];
        if (provider == nullptr)
            return String();

        screen->behavior = (*provider)();
        return String(2000 * 1000);
    }, [screen](){
        if (screen->behavior)
            return screen->behavior->name();
        return String("None");
    });

    _server.on("/behavior/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", CrudeJson::array(NativeBehaviors::list.keys));
    });

    _server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(5000 * 1000));
        ESP.restart();
    });

    _server.on("/checkupdate", HTTP_POST, [updater](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", String(updater->check()));
    });

    _server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", WifiLog.output.string());
    });
    
    registerREST("/brightness", "brightness", [screen](String value) {
        screen->setBrightness(value.toFloat());
        return "Success";
    }, [screen]() { return String(screen->getBrightness()); });

    registerREST("/response", "response", [screen](String value) {
        screen->setResponse(value.toFloat());
        return "Success";
    }, [screen]() { return String(screen->getResponse()); });

    // -----------------------------------------------
    // ------------------- Data ----------------------
    // -----------------------------------------------

    _server.on("/i", HTTP_GET,[videoInterface](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        videoInterface->info(response);
        request->send(response);
    });
}

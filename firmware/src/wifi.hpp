// wifi.hpp

#pragma once

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "./future.hpp"

class Wifi {
public:
    tl::expected<String, Error> ip() const {
        if (WiFi.status() != WL_CONNECTED)
            return tl::make_unexpected(Error("not connected"));
        return WiFi.localIP().toString();
    }

    SharedFuture<int> post(const String& url, const String& body) {
        return SharedFuture<int>([url, body]() -> SharedFuture<int>::Result {
            if (WiFi.status() != WL_CONNECTED)
                return tl::make_unexpected(Error("not connected"));

            HTTPClient http;
            WiFiClientSecure client;

            client.setInsecure();
            http.begin(client, url);
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Connection", "close");
            
            int code = http.POST(body);
            if (code >= 0) return code;
            Serial.println("Response: " + http.getString());
            http.end();

            return tl::make_unexpected(Error(HTTPClient::errorToString(code)));
        }, "wifi_post", 16384);
    }

    SharedFuture<bool> open(const char* ssid, const char* pass) {
        return SharedFuture<bool>([ssid, pass]() -> SharedFuture<bool>::Result {
            EventGroupHandle_t eg = xEventGroupCreate();

            WiFi.onEvent([eg](arduino_event_id_t event, arduino_event_info_t) {
                xEventGroupSetBits(eg, 1);
            }, ARDUINO_EVENT_WIFI_STA_GOT_IP);

            WiFi.begin(ssid, pass);
            xEventGroupWaitBits(eg, 1, pdFALSE, pdTRUE, portMAX_DELAY);

            WiFi.removeEvent(ARDUINO_EVENT_WIFI_STA_GOT_IP);
            vEventGroupDelete(eg);
            return true;
        }, "wifi_connect", 4096);
    }
};

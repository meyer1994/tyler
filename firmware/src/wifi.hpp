// wifi.hpp

#pragma once

#include <WiFi.h>
#include "./future.hpp"

class Wifi {
public:
    tl::expected<String, Error> ip() const {
        if (WiFi.status() != WL_CONNECTED)
            return tl::make_unexpected(Error("not connected"));
        return WiFi.localIP().toString();
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

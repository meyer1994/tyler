// cellular.hpp

#pragma once

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <memory>
#include <string>
#include <utility>
#include "./result.hpp"

// https://github.com/jbasu2013/T-A7670X/blob/main/examples/HttpClient/utilities.h#L18-L35
#define MODEM_BAUDRATE                      (115200)
#define MODEM_DTR_PIN                       (25)
#define MODEM_TX_PIN                        (26)
#define MODEM_RX_PIN                        (27)
#define BOARD_PWRKEY_PIN                    (4)
#define BOARD_ADC_PIN                       (35)
#define BOARD_POWERON_PIN                   (12)
#define MODEM_RING_PIN                      (33)
#define MODEM_RESET_PIN                     (5)
#define BOARD_MISO_PIN                      (2)
#define BOARD_MOSI_PIN                      (15)
#define BOARD_SCK_PIN                       (14)
#define BOARD_SD_CS_PIN                     (13)
#define BOARD_BAT_ADC_PIN                   (35)
#define MODEM_RESET_LEVEL                   HIGH
#define SerialAT                            Serial2

struct HttpResponse {
    int code;
    JsonDocument body;
};

class Cellular {
public:
    Cellular() {}

    tl::expected<String, Error> ip() const {
        if (!modem_) return tl::make_unexpected(Error("not initialized"));
        String addr = modem_->getLocalIP();
        if (addr.isEmpty()) return tl::make_unexpected(Error("no IP"));
        return addr;
    }

    Result<HttpResponse> post(const String& host, int port,
                              const String& path, const String& request_body) {
        auto modem = modem_;
        if (!modem) return tl::make_unexpected(Error("not initialized"));

        TinyGsmClient client(*modem);
        HttpClient http(client, host, port);

        Serial.println("[cell] POST " + host + ":" + String(port) + path);
        http.post(path, "application/json", request_body);

        const int code = http.responseStatusCode();
        http.skipResponseHeaders();
        const String response_body = http.responseBody();

        Serial.println("[cell] response: " + response_body);
        http.stop();

        if (code < 0)
            return tl::make_unexpected(Error("HTTP error: " + String(code)));

        HttpResponse response;
        response.code = code;

        DeserializationError error =
            deserializeJson(response.body, response_body);
        if (error) {
            Serial.println("[cell] JSON parse error: " +
                           String(error.c_str()));
            return tl::make_unexpected(
                Error("JSON parse error: " + String(error.c_str())));
        }
        return response;
    }

    Result<bool> open(const char* apn) {
        std::string apn_str(apn);

        pinMode(BOARD_POWERON_PIN, OUTPUT);
        digitalWrite(BOARD_POWERON_PIN, HIGH);
        pinMode(MODEM_RESET_PIN, OUTPUT);
        digitalWrite(MODEM_RESET_PIN, LOW);
        pinMode(MODEM_DTR_PIN, OUTPUT);
        digitalWrite(MODEM_DTR_PIN, LOW);
        pinMode(BOARD_PWRKEY_PIN, OUTPUT);
        digitalWrite(BOARD_PWRKEY_PIN, LOW);
        delay(100);
        digitalWrite(BOARD_PWRKEY_PIN, HIGH);
        delay(1000);
        digitalWrite(BOARD_PWRKEY_PIN, LOW);

        SerialAT.begin(
            MODEM_BAUDRATE, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
        modem_ = std::make_shared<TinyGsm>(SerialAT);

        Serial.println("[cell] powering on modem...");
        if (!modem_->testAT(10000))
            return tl::make_unexpected(Error("modem not responding"));
        Serial.println("[cell] modem OK");

        Serial.println("[cell] initializing modem...");
        if (!modem_->init())
            return tl::make_unexpected(Error("modem init failed"));

        Serial.println("[cell] waiting for network...");
        if (!modem_->waitForNetwork(60000))
            return tl::make_unexpected(
                Error("network registration failed"));

        Serial.println("[cell] operator: " + modem_->getOperator());
        Serial.println(
            "[cell] connecting GPRS (apn: " +
            String(apn_str.c_str()) + ")...");
        if (!modem_->gprsConnect(apn_str.c_str()))
            return tl::make_unexpected(Error("GPRS connect failed"));

        Serial.println("[cell] GPRS connected!");
        return true;
    }

private:
    std::shared_ptr<TinyGsm> modem_;
};

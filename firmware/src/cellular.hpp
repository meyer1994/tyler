// cellular.hpp

#pragma once

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <string>
#include "./future.hpp"

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

class Cellular {
public:
    Cellular() {}

    tl::expected<String, Error> ip() const {
        if (!modem_) return tl::make_unexpected(Error("not initialized"));
        String addr = modem_->getLocalIP();
        if (addr.isEmpty()) return tl::make_unexpected(Error("no IP"));
        return addr;
    }

    SharedFuture<int> post(const String& host, int port,
                           const String& path, const String& body) {
        auto modem = modem_;
        return SharedFuture<int>([modem, host, port, path, body]() -> SharedFuture<int>::Result {
            if (!modem) return tl::make_unexpected(Error("not initialized"));

            TinyGsmClient client(*modem);
            HttpClient http(client, host, port);

            http.post(path, "application/json", body);
            int code = http.responseStatusCode();
            http.skipResponseHeaders();
            Serial.println("Response: " + http.responseBody());
            http.stop();

            if (code >= 0) return code;
            return tl::make_unexpected(Error("HTTP error: " + String(code)));
        }, "cell_post", 16384);
    }

    SharedFuture<bool> open(const char* apn) {
        std::string apn_str(apn);
        return SharedFuture<bool>([this, apn_str]() -> SharedFuture<bool>::Result {
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

            SerialAT.begin(MODEM_BAUDRATE, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
            modem_ = std::make_shared<TinyGsm>(SerialAT);

            if (!modem_->testAT(10000)) return tl::make_unexpected(Error("modem not responding"));

            Serial.println("[cell] initializing modem...");
            if (!modem_->init()) return tl::make_unexpected(Error("modem init failed"));

            Serial.println("[cell] waiting for network...");
            if (!modem_->waitForNetwork(60000)) return tl::make_unexpected(Error("network registration failed"));

            Serial.println("[cell] operator: " + modem_->getOperator());
            Serial.println("[cell] connecting GPRS (apn: " + String(apn_str.c_str()) + ")...");
            if (!modem_->gprsConnect(apn_str.c_str())) return tl::make_unexpected(Error("GPRS connect failed"));

            Serial.println("[cell] GPRS connected!");
            return true;
        }, "cell_connect", 12288);
    }

private:
    std::shared_ptr<TinyGsm> modem_;
};

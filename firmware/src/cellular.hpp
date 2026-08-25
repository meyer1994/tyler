// cellular.hpp

#pragma once

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <future>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <esp_pthread.h>
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
    uint16_t code;
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

    std::future<Result<HttpResponse>> post(const String& host, int port,
                           const String& path, const String& body) {
        auto modem = modem_;
        return runAsync<HttpResponse>([modem, host, port, path, body]() -> Result<HttpResponse> {
            if (!modem) return tl::make_unexpected(Error("not initialized"));

            TinyGsmClient client(*modem);
            HttpClient http(client, host, port);

            Serial.println("[cell] POST " + host + ":" + String(port) + path);
            http.post(path, "application/json", body);

            uint16_t code = http.responseStatusCode();
            http.skipResponseHeaders();
            String body = http.responseBody();

            Serial.println("[cell] response: " + body);
            http.stop();

            HttpResponse response;
            response.code = code;

            DeserializationError error = deserializeJson(response.body, body);
            if (error) {
                Serial.println("[cell] JSON parse error: " + String(error.c_str()));
                return tl::make_unexpected(Error("JSON parse error: " + String(error.c_str())));
            }

            if (code >= 0) return response;
            return tl::make_unexpected(Error("HTTP error: " + String(code)));
        }, "cell_post", 16384);
    }

    std::future<Result<bool>> open(const char* apn) {
        std::string apn_str(apn);
        return runAsync<bool>([this, apn_str]() -> Result<bool> {
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

            Serial.println("[cell] powering on modem...");
            if (!modem_->testAT(10000)) return tl::make_unexpected(Error("modem not responding"));
            Serial.println("[cell] modem OK");

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
    template <typename T, typename Fn>
    static std::future<Result<T>> runAsync(
        Fn&& fn, const char* name, size_t stack_size) {

        std::packaged_task<Result<T>()> task(std::forward<Fn>(fn));
        auto future = task.get_future();

        esp_pthread_cfg_t previous{};
        const bool had_previous = esp_pthread_get_cfg(&previous) == ESP_OK;
        const esp_pthread_cfg_t restore =
            had_previous ? previous : esp_pthread_get_default_config();

        esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
        cfg.stack_size = stack_size;
        cfg.prio = 1;
        cfg.inherit_cfg = false;
        cfg.thread_name = name;
        cfg.pin_to_core = tskNO_AFFINITY;

        if (esp_pthread_set_cfg(&cfg) != ESP_OK)
            throw std::runtime_error("cellular thread configuration failed");

        try {
            std::thread worker(std::move(task));
            worker.detach();
            esp_pthread_set_cfg(&restore);
        } catch (...) {
            esp_pthread_set_cfg(&restore);
            throw;
        }
        return future;
    }

    std::shared_ptr<TinyGsm> modem_;
};

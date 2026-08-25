#include <Arduino.h>
#include <esp_task_wdt.h>
#if defined(TRACKOWS_GPS_SIM7000G)
#include <Ticker.h>
#endif

#include "./events.hpp"
#include "./gps.hpp"
#include "./http.hpp"
#include "./network.hpp"

#ifndef WIFI_SSID
#error "WIFI_SSID is not defined"
#endif

#ifndef WIFI_PASS
#error "WIFI_PASS is not defined"
#endif

#ifndef SERVER_URL
#error "SERVER_URL is not defined"
#endif

namespace {

GpsEmitter gps_events("gps_events", 16, 4096, 1);
HttpEmitter http_events("http_events", 24, 4096, 1);
NetworkEmitter network_events("network_events", 8, 4096, 1);
AppEmitter app_events("app_events", 16, 4096, 1);

GPS gps(gps_events);
NetworkService network(network_events, WIFI_SSID, WIFI_PASS);
HttpService http(http_events, SERVER_URL);

#if defined(TRACKOWS_GPS_SIM7000G)
constexpr uint8_t RESPONSE_LED_PIN = 12;
constexpr uint32_t RESPONSE_LED_DURATION_MS = 100;
Ticker response_led_timer;
#endif

void stopServices() {
    (void)gps.stop();
    (void)http.stop();
    (void)network.stop();
}

bool startEventLoops() {
    struct Loop {
        const char* name;
        esp_err_t result;
    };

    const Loop loops[]{
        {"gps", gps_events.start()},
        {"http", http_events.start()},
        {"network", network_events.start()},
        {"app", app_events.start()},
    };
    for (const auto& loop : loops) {
        if (loop.result == ESP_OK) continue;
        Serial.println(
            "[main] failed to start " + String(loop.name) +
            " event loop: " + esp_err_to_name(loop.result));
        return false;
    }
    return true;
}

void relayAppError(const EventError& error, void* context) {
    auto* events = static_cast<AppEmitter*>(context);
    (void)events->emit<AppEvent::Error>(error);
}

void registerHandlers() {
    (void)app_events.on<AppEvent::Started>(
        [](const NoPayload&, void*) {
            Serial.println("[main] streaming every GPS fix");
        });

    (void)app_events.on<AppEvent::Error>(
        [](const EventError& error, void*) {
            Serial.println(
                "[app] error (" + String(error.code) + "): " +
                error.message);
        });

    (void)gps_events.on<GpsEvent::Error>(relayAppError, &app_events);
    (void)http_events.on<HttpEvent::Error>(relayAppError, &app_events);
    (void)network_events.on<NetworkEvent::Error>(relayAppError, &app_events);

    (void)gps_events.on<GpsEvent::StateChanged>(
        [](const GPSStateChange& state, void*) {
            Serial.println(
                "[gps] state: " + String(state.previous.getName()) +
                " -> " + state.current.getName());
        });

    (void)gps_events.on<GpsEvent::FixStateChanged>(
        [](const GPSFixStateChange& state, void*) {
            Serial.println(
                "[gps] fix state: " + String(state.previous.getName()) +
                " -> " + state.current.getName());
        });

    (void)gps_events.on<GpsEvent::Fix>(
        [](const GPSFix& fix, void* context) {
            Serial.println("[gps] fix: " + fix.toJson());
            auto* events = static_cast<HttpEmitter*>(context);
            const esp_err_t result =
                events->emit<HttpEvent::PostLocation>({fix});
            if (result != ESP_OK)
                Serial.println(
                    "[gps] HTTP event dropped: " +
                    String(esp_err_to_name(result)));
        },
        &http_events);

    (void)network_events.on<NetworkEvent::StateChanged>(
        [](const NetworkStateChange& state, void*) {
            Serial.println(
                "[network] state: " +
                String(static_cast<int>(state.previous)) + " -> " +
                String(static_cast<int>(state.current)));
        });

    (void)network_events.on<NetworkEvent::AddressAcquired>(
        [](const NetworkAddress& address, void*) {
            Serial.println("[network] IP: " + String(address.address));
        });

#if defined(TRACKOWS_GPS_SIM7000G)
    (void)http_events.on<HttpEvent::Response>(
        [](const HttpResponse& response, void*) {
            if (response.code < 200 || response.code >= 300) return;
            digitalWrite(RESPONSE_LED_PIN, LOW);
            response_led_timer.once_ms(RESPONSE_LED_DURATION_MS, [] {
                digitalWrite(RESPONSE_LED_PIN, HIGH);
            });
        });
#endif
}

}  // namespace

void setup() {
    Serial.begin(115200);
    Serial.println("[main] starting up...");

    delay(2000);
    esp_task_wdt_deinit();

#if defined(TRACKOWS_GPS_SIM7000G)
    pinMode(RESPONSE_LED_PIN, OUTPUT);
    digitalWrite(RESPONSE_LED_PIN, HIGH);  // Active-low LED: start off.
#endif

    if (!startEventLoops()) return;
    registerHandlers();

    auto network_result = network.start();
    if (!network_result.has_value()) {
        Serial.println(
            "[main] network start failed: " +
            network_result.error().message);
        stopServices();
        return;
    }

    auto http_result = http.start();
    if (!http_result.has_value()) {
        Serial.println(
            "[main] HTTP start failed: " + http_result.error().message);
        stopServices();
        return;
    }

    auto gps_result = gps.start();
    if (!gps_result.has_value()) {
        Serial.println(
            "[main] GPS start failed: " + gps_result.error().message);
        stopServices();
        return;
    }

    (void)app_events.emit<AppEvent::Started>();
}

void loop() {
    // Services, event dispatch, and HTTP workers own their FreeRTOS tasks.
    delay(1000);
}

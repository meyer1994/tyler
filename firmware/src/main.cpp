#include <Arduino.h>
#include <esp_task_wdt.h>

#include "./events.hpp"
#include "./gps.hpp"
#include "./http.hpp"
#include "./network.hpp"

#if defined(TRACKOWS_BOARD_SIM7000G)
#include "./gps/sim7000g.hpp"
#include <Ticker.h>
using GpsBackendImplementation = Sim7000GpsBackend;
#else
#include "./gps/external.hpp"
using GpsBackendImplementation = ExternalGpsBackend;
#endif

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

GpsBackendImplementation gps_backend;
EventEmitter events(5);

GPS gps(events, gps_backend);
NetworkService network(events, WIFI_SSID, WIFI_PASS);
HttpService http(events, SERVER_URL);

#if defined(TRACKOWS_BOARD_SIM7000G)
constexpr uint8_t RESPONSE_LED_PIN = 12;
constexpr uint32_t RESPONSE_LED_DURATION_MS = 100;
Ticker response_led_timer;
#endif

void stopServices() {
  (void)gps.stop();
  (void)http.stop();
  (void)network.stop();
}

void registerHandlers() {
  events.on(GpsEvent::Error, [](void* data) {
    const auto& error = *static_cast<EventError*>(data);
    Serial.println(
        "[event] gps.error (" + String(error.code) + "): " +
        error.message);
  });

  events.on(GpsEvent::StateChanged, [](void* data) {
    const auto& state = *static_cast<GPSStateChange*>(data);
    Serial.println(
        "[event] gps.state_changed: " +
        String(state.previous.getName()) + " -> " +
        state.current.getName());
  });

  events.on(GpsEvent::FixStateChanged, [](void* data) {
    const auto& state = *static_cast<GPSFixStateChange*>(data);
    Serial.println(
        "[event] gps.fix_state_changed: " +
        String(state.previous.getName()) + " -> " +
        state.current.getName());
  });

  events.on(GpsEvent::Fix, [](void* data) {
    const auto& fix = *static_cast<GPSFix*>(data);
    Serial.println("[event] gps.fix: " + fix.toJson());

    if (!events.emit(HttpEvent::PostLocation, HttpPostLocation{fix}))
      Serial.println("[event] http.post_location dropped");
  });

  events.on(HttpEvent::PostLocation, [](void*) {
    Serial.println("[event] http.post_location");
  });

  events.on(HttpEvent::Response, [](void* data) {
    const auto& response = *static_cast<HttpResponse*>(data);
    Serial.println(
        "[event] http.response: " + String(response.code));
    if (response.code == 200)
        Serial.println("[event] http.response.json: " + response.body);

#if defined(TRACKOWS_BOARD_SIM7000G)
    if (response.code < 200 || response.code >= 300) return;
    digitalWrite(RESPONSE_LED_PIN, LOW);
    response_led_timer.once_ms(
        RESPONSE_LED_DURATION_MS,
        [] { digitalWrite(RESPONSE_LED_PIN, HIGH); });
#endif
  });

  events.on(HttpEvent::Error, [](void* data) {
    const auto& error = *static_cast<EventError*>(data);
    Serial.println(
        "[event] http.error (" + String(error.code) + "): " +
        error.message);
  });

  events.on(NetworkEvent::StateChanged, [](void* data) {
    const auto& state = *static_cast<NetworkStateChange*>(data);
    Serial.println(
        "[event] network.state_changed: " +
        String(static_cast<int>(state.previous)) + " -> " +
        String(static_cast<int>(state.current)));
  });

  events.on(NetworkEvent::AddressAcquired, [](void* data) {
    const auto& address = *static_cast<NetworkAddress*>(data);
    Serial.println(
        "[event] network.address_acquired: " +
        String(address.address));
  });

  events.on(NetworkEvent::Error, [](void* data) {
    const auto& error = *static_cast<EventError*>(data);
    Serial.println(
        "[event] network.error (" + String(error.code) + "): " +
        error.message);
  });
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println("[main] starting up...");

  delay(2000);
  esp_task_wdt_deinit();

#if defined(TRACKOWS_BOARD_SIM7000G)
  pinMode(RESPONSE_LED_PIN, OUTPUT);
  digitalWrite(RESPONSE_LED_PIN, HIGH);  // Active-low LED: start off.
#endif

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
        "[main] HTTP start failed: " +
        http_result.error().message);
    stopServices();
    return;
  }

  auto gps_result = gps.start();
  if (!gps_result.has_value()) {
    Serial.println(
        "[main] GPS start failed: " +
        gps_result.error().message);
    stopServices();
    return;
  }
}

void loop() {
  // Services and event dispatch own their FreeRTOS tasks.
  delay(1000);
}

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "./events.hpp"
#include "./gps.hpp"
#include "./http.hpp"
#include "./network/wifi.hpp"

#if defined(TRACKOWS_BOARD_SIM7000G)
#include "./gps/sim7000g.hpp"
using GpsBackendImplementation = Sim7000GpsBackend;
#else
#include "./gps/external.hpp"
using GpsBackendImplementation = ExternalGpsBackend;
#endif

#if defined(TRACKOWS_BOARD_SIM7000G) || defined(TRACKOWS_BOARD_A7670G)
#include <Ticker.h>
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

GPS gps(gps_backend);
WifiNetwork network;
HttpService http(network, SERVER_URL);

#if defined(TRACKOWS_BOARD_SIM7000G)
constexpr uint8_t RESPONSE_LED_PIN = 12;
constexpr uint32_t RESPONSE_LED_DURATION_MS = 100;
constexpr uint32_t INTERSECT_LED_DURATION_MS = 40;
Ticker response_led_timer;
uint8_t response_led_transitions = 0;
uint32_t response_led_duration_ms = RESPONSE_LED_DURATION_MS;

void finishResponseLedBlink() {
  digitalWrite(
      RESPONSE_LED_PIN,
      response_led_transitions % 2 ? HIGH : LOW);
  --response_led_transitions;
  if (response_led_transitions > 0)
    response_led_timer.once_ms(
        response_led_duration_ms,
        finishResponseLedBlink);
}

void blinkResponseLed(bool intersects) {
  digitalWrite(RESPONSE_LED_PIN, LOW);
  response_led_transitions = intersects ? 3 : 1;
  response_led_duration_ms = intersects
      ? INTERSECT_LED_DURATION_MS
      : RESPONSE_LED_DURATION_MS;
  response_led_timer.once_ms(
      response_led_duration_ms,
      finishResponseLedBlink);
}
#endif

#if defined(TRACKOWS_BOARD_A7670G)
constexpr uint8_t ALARM_PIN = 32;
constexpr uint32_t ALARM_DURATION_MS = 1000;
Ticker alarm_timer;

void pulseAlarm() {
  digitalWrite(ALARM_PIN, HIGH);
  alarm_timer.once_ms(ALARM_DURATION_MS, [] {
    digitalWrite(ALARM_PIN, LOW);
  });
}
#endif

void stopServices() {
  (void)gps.stop();
  (void)http.stop();
}

void registerHandlers() {
  gps.events().on(GpsEvent::Error, [](void* data) {
    const auto& error = *static_cast<EventError*>(data);
    Serial.println(
        "[event] gps.error (" + String(error.code) + "): " +
        error.message);
  });

  gps.events().on(GpsEvent::StateChanged, [](void* data) {
    const auto& state = *static_cast<GPSStateChange*>(data);
    Serial.println(
        "[event] gps.state_changed: " +
        String(state.previous.getName()) + " -> " +
        state.current.getName());
  });

  gps.events().on(GpsEvent::FixStateChanged, [](void* data) {
    const auto& state = *static_cast<GPSFixStateChange*>(data);
    Serial.println(
        "[event] gps.fix_state_changed: " +
        String(state.previous.getName()) + " -> " +
        state.current.getName());
  });

  gps.events().on(GpsEvent::Fix, [](void* data) {
    const auto& fix = *static_cast<GPSFix*>(data);
    Serial.println("[event] gps.fix: " + fix.toJson());

    if (!http.events().emit(HttpEvent::PostLocation, HttpPostLocation{fix}))
      Serial.println("[event] http.post_location dropped");
  });

  http.events().on(HttpEvent::PostLocation, [](void*) {
    Serial.println("[event] http.post_location");
  });

  http.events().on(HttpEvent::Response, [](void* data) {
    const auto& response = *static_cast<HttpResponse*>(data);
    Serial.println(
        "[event] http.response: code=" + String(response.code) +
        " ok=" + String(response.ok ? "true" : "false") +
        " intersects=" +
        String(response.intersects ? "true" : "false"));

#if defined(TRACKOWS_BOARD_SIM7000G)
    if (response.code < 200 || response.code >= 300) return;
    blinkResponseLed(response.intersects);
#endif

#if defined(TRACKOWS_BOARD_A7670G)
    if (response.code < 200 || response.code >= 300) return;
    if (response.ok && !response.intersects) pulseAlarm();
#endif
  });

  http.events().on(HttpEvent::Error, [](void* data) {
    const auto& error = *static_cast<EventError*>(data);
    Serial.println(
        "[event] http.error (" + String(error.code) + "): " +
        error.message);
  });
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println("[main] starting up...");

  delay(2000);
  esp_task_wdt_deinit();

  Serial.println("[main] MAC: " + network.macAddress());

#if defined(TRACKOWS_BOARD_SIM7000G)
  pinMode(RESPONSE_LED_PIN, OUTPUT);
  digitalWrite(RESPONSE_LED_PIN, HIGH);  // Active-low LED: start off.
#endif

#if defined(TRACKOWS_BOARD_A7670G)
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);
#endif

  registerHandlers();

  auto network_result = network.init(WIFI_SSID, WIFI_PASS);
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

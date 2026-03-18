#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include "./future.hpp"

#define GPS_RX_PIN        22
#define GPS_TX_PIN        -1
#define GPS_BAUD          9600
#define WIFI_SSID         "my-wifi"
#define WIFI_PASS         "my-password"
#define SLEEP_INTERVAL_US (10ULL * 1000000ULL)

struct GpsFix {
    double   lat;
    double   lng;
    uint32_t sats;
};

// ── WiFiManager ─────────────────────────────────────────────────

class WiFiManager {
public:
    // Returns future that resolves when connected, rejects on failure
    Future<bool> open(const char* ssid, const char* pass) {
        _connectPromise = Promise<bool>();
        _ipPromise      = Promise<IPAddress>();
        WiFi.mode(WIFI_STA);
        _handlerId = WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t) {
            _onEvent(event);
        });
        WiFi.begin(ssid, pass);
        return _connectPromise.get_future();
    }

    // Returns future that resolves when disconnected
    Future<bool> close() {
        _removeHandler();
        _disconnectPromise = Promise<bool>();
        _handlerId = WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t) {
            if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
                _removeHandler();
                _disconnectPromise.resolve(true);
            }
        });
        WiFi.disconnect(true);
        return _disconnectPromise.get_future();
    }

    Future<IPAddress> ip() { return _ipPromise.get_future(); }

private:
    Promise<bool>      _connectPromise;
    Promise<bool>      _disconnectPromise;
    Promise<IPAddress> _ipPromise;
    wifi_event_id_t    _handlerId = -1;

    void _removeHandler() {
        if (_handlerId != -1) {
            WiFi.removeEvent(_handlerId);
            _handlerId = -1;
        }
    }

    void _onEvent(WiFiEvent_t event) {
        switch (event) {
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                _connectPromise.resolve(true);
                _ipPromise.resolve(WiFi.localIP());
                break;
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                _connectPromise.reject("disconnected");
                break;
            default:
                break;
        }
    }
};

// ── GpsManager ──────────────────────────────────────────────────

class GpsManager {
public:
    GpsManager() : _serial(2), _task(nullptr) {}

    // Returns future that resolves on first valid fix
    Future<bool> open() {
        _connectPromise = Promise<bool>();
        _fixPromise     = Promise<GpsFix>();
        _running        = true;
        _serial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        xTaskCreate(_entry, "gps", 8192, this, 1, &_task);
        return _connectPromise.get_future();
    }

    // Signals task to stop; returns future that resolves when task exits
    Future<bool> close() {
        _stopPromise = Promise<bool>();
        _running     = false;
        return _stopPromise.get_future();
    }

    Future<GpsFix> fix() { return _fixPromise.get_future(); }

private:
    HardwareSerial  _serial;
    TaskHandle_t    _task;
    TinyGPSPlus     _gps;
    Promise<bool>   _connectPromise;
    Promise<bool>   _stopPromise;
    Promise<GpsFix> _fixPromise;
    std::atomic<bool> _running { false };

    static void _entry(void* arg) {
        static_cast<GpsManager*>(arg)->_run();
        vTaskDelete(nullptr);
    }

    void _run() {
        while (_running.load()) {
            while (_serial.available()) _gps.encode(_serial.read());

            if (_gps.location.isValid() && _gps.location.isUpdated()) {
                GpsFix f = {
                    .lat  = _gps.location.lat(),
                    .lng  = _gps.location.lng(),
                    .sats = _gps.satellites.value()
                };
                _connectPromise.resolve(true);
                _fixPromise.resolve(f);
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        _serial.end();
        _stopPromise.resolve(true);
    }
};

// ── globals ─────────────────────────────────────────────────────
static GpsManager  gps;
static WiFiManager wifi;

void deepSleep() {
    gps.close().get(5000);
    wifi.close().get(5000);
    esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_US);
    Serial.printf("Sleeping for %llu s\r\n", SLEEP_INTERVAL_US / 1000000ULL);
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting up...");

    auto gpsConnected  = gps.open();
    auto wifiConnected = wifi.open(WIFI_SSID, WIFI_PASS);
    auto gpsFix        = gps.fix();
    auto wifiIp        = wifi.ip();

    if (!gpsConnected.get(90000).ok()) {
        Serial.println("GPS fix failed");
        deepSleep();
    }
    Serial.println("GPS fix acquired");

    if (!wifiConnected.get(15000).ok()) {
        Serial.println("WiFi connection failed");
        deepSleep();
    }
    Serial.println("WiFi connected");

    const GpsFix    fix = *gpsFix.get().value;
    const IPAddress ip  = *wifiIp.get().value;
    Serial.printf("IP:  %s\r\n", ip.toString().c_str());
    Serial.printf("GPS: lat=%f, lng=%f, sats=%d\r\n", fix.lat, fix.lng, fix.sats);

    deepSleep();
}

void loop() {
    vTaskDelete(NULL);
}
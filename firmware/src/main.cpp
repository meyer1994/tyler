#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_sleep.h>

// ── L76K pins ───────────────────────────────────────────────────
#define GPS_RX_PIN   22
#define GPS_TX_PIN   -1
#define GPS_BAUD     9600
// ── credentials ─────────────────────────────────────────────────
#define WIFI_SSID "my-wifi"
#define WIFI_PASS "my-password"
// ── server URL ──────────────────────────────────────────────────
#define SERVER_URL "https://trackows.meyer1994.workers.dev/api/send"
// ── deep sleep ──────────────────────────────────────────────────
#define SLEEP_INTERVAL_US (10ULL * 1000000ULL)   // 10 seconds


struct GpsFix {
    double lat;
    double lng;
    uint32_t sats;
};


class WiFiManager {
    public:
      WiFiManager():
        _events(nullptr),
        _connected(false)
        {}

      void start() {
        _events = xEventGroupCreate();

        WiFi.mode(WIFI_STA);
        WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t) {
          this->_eventHandler(event);
        });
      }

      void connect(const char *ssid, const char *pass) {
        WiFi.begin(ssid, pass);
      }

      // Blocks caller until connected or timeout; returns success
      const bool waitConnected(const uint32_t timeoutMs = 15000) {
        EventBits_t bits = xEventGroupWaitBits(
          _events,
          CONNECTED_BIT | DISCONNECTED_BIT,
          pdFALSE, pdFALSE,
          pdMS_TO_TICKS(timeoutMs)
        );
        _connected = (bits & CONNECTED_BIT) != 0;
        return _connected;
      }

      const bool connected() { return _connected; }
      const IPAddress ip()   { return WiFi.localIP(); }

      void disconnect() {
        WiFi.disconnect(true);
      }

      // Blocks caller until disconnected or timeout; returns true if disconnected
      const bool waitDisconnected(const uint32_t timeoutMs = 15000) {
        EventBits_t bits = xEventGroupWaitBits(
          _events,
          DISCONNECTED_BIT,
          pdFALSE, pdFALSE,
          pdMS_TO_TICKS(timeoutMs)
        );

        bool disconnected = (bits & DISCONNECTED_BIT) != 0;
        if (disconnected) _connected = false;
        return disconnected;
      }

    private:
      static constexpr EventBits_t CONNECTED_BIT    = BIT0;
      static constexpr EventBits_t DISCONNECTED_BIT = BIT1;

      EventGroupHandle_t _events;
      bool               _connected;

      void _eventHandler(WiFiEvent_t event) {
        switch (event) {
          case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            xEventGroupSetBits(_events, CONNECTED_BIT);
            break;
          case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            xEventGroupSetBits(_events, DISCONNECTED_BIT);
            break;
          default:
            break;
        }
      }
};


class GpsManager {
public:
    GpsManager():
        _events(nullptr),
        _serial(2),
        _task(nullptr),
        _hasFix(false),
        _fix{}
    {}

    void start() {
        _events = xEventGroupCreate();
        _serial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        xTaskCreate(_entry, "gps", 8192, this, 1, &_task);
    }

    void stop() {
        xEventGroupSetBits(_events, STOP_BIT);
    }

    const bool waitFix(const uint32_t timeoutMs = 60000) {
        EventBits_t bits = xEventGroupWaitBits(
            _events,
            FIX_BIT,
            pdFALSE, pdFALSE,
            pdMS_TO_TICKS(timeoutMs)
        );
        return (bits & FIX_BIT) != 0;
    }

    const bool waitStopped(const uint32_t timeoutMs = 5000) {
        EventBits_t bits = xEventGroupWaitBits(
            _events,
            STOPPED_BIT,
            pdFALSE, pdFALSE,
            pdMS_TO_TICKS(timeoutMs)
        );
        return (bits & STOPPED_BIT) != 0;
    }

    const bool   connected() { return _hasFix; }
    const GpsFix fix()       { return _fix; }

private:
    static constexpr EventBits_t FIX_BIT     = BIT0;
    static constexpr EventBits_t STOP_BIT    = BIT1;
    static constexpr EventBits_t STOPPED_BIT = BIT2;

    static constexpr uint32_t FIX_TIMEOUT = 1000;

    EventGroupHandle_t _events;
    HardwareSerial     _serial;
    TaskHandle_t       _task;
    bool               _hasFix;
    TinyGPSPlus        _gps;
    GpsFix             _fix;

    static void _entry(void* arg) {
        static_cast<GpsManager*>(arg)->_run();
    }

    void _sleep(const uint32_t timeoutMs = 1000) {
        vTaskDelay(pdMS_TO_TICKS(timeoutMs));
    }

    void _run() {
        while (true) {
            if (xEventGroupGetBits(_events) & STOP_BIT) break;
            while (_serial.available()) _gps.encode(_serial.read());

            if (!_gps.location.isValid()) {
                _sleep(FIX_TIMEOUT);
                continue;
            }

            if (!_gps.location.isUpdated()) {
                _sleep(FIX_TIMEOUT);
                continue;
            }

            _fix = {
                .lat = _gps.location.lat(),
                .lng = _gps.location.lng(),
                .sats = _gps.satellites.value()
            };
            _hasFix = true;

            Serial.printf("GPS: fix: lat=%f, lng=%f, sats=%d\r\n",
                 _fix.lat, _fix.lng, _fix.sats);

            xEventGroupSetBits(_events, FIX_BIT);
            _sleep(FIX_TIMEOUT);
        }

        xEventGroupSetBits(_events, STOPPED_BIT);
        vTaskDelete(NULL);

        _serial.end();
        _task = nullptr;
    }
};


static GpsManager gps;
static WiFiManager wifi;

void deepSleep() {
    gps.stop();
    wifi.disconnect();

    if (!wifi.waitDisconnected()) {
        Serial.println("WiFi disconnect failed");
    } else {
        Serial.println("WiFi disconnected");
    }

    if (!gps.waitStopped()) {
        Serial.println("GPS stop failed");
    } else {
        Serial.println("GPS stopped");
    }

    esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_US);
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting up...");

    gps.start();
    wifi.start();
    wifi.connect(WIFI_SSID, WIFI_PASS);

    if (!gps.waitFix(90000)) {
        Serial.println("GPS fix failed");
        deepSleep();
    }
    Serial.println("GPS fix acquired");
    gps.stop();

    if (!wifi.waitConnected()) {
        Serial.println("WiFi connection failed");
        deepSleep();
    }
    Serial.println("WiFi connected");

    const GpsFix fix = gps.fix();
    const IPAddress ip = wifi.ip();
    Serial.printf("IP address: %s\r\n", ip.toString().c_str());
    Serial.printf("GPS fix: %f, %f, %d\r\n", fix.lat, fix.lng, fix.sats);

    Serial.printf("Entering deep sleep for %llu seconds\r\n", SLEEP_INTERVAL_US / 1000000ULL);
    deepSleep();
}

void loop() {
    vTaskDelete(NULL);
}

#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define GPS_RX_PIN  22
#define GPS_TX_PIN  -1
#define GPS_BAUD    9600
#define BUTTON_PIN  0

const char* WIFI_SSID     = "my-network";
const char* WIFI_PASSWORD = "my-password";
const char* SERVER_URL    = "https://trackows.meyer1994.workers.dev/api/send";
const char* BOARD_ID      = "1234567890";

TinyGPSPlus gps;
HardwareSerial SerialGPS(2);

bool lastButtonState = HIGH;
uint32_t lastDebugMs = 0;
uint32_t lastSendMs = 0;

// ── Init ─────────────────────────────────────────────────────────────────────

void initGPS() {
    SerialGPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("GPS initialized.");
}

void initWifi() {
    Serial.printf("Connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
}

// ── Handlers ─────────────────────────────────────────────────────────────────

void handleGPS() {
    Serial.println("========== GPS DATA ==========");

    Serial.print("Fix valid:   "); Serial.println(gps.location.isValid() ? "YES" : "NO");

    Serial.print("Latitude:    ");
    Serial.println(gps.location.isValid() ? String(gps.location.lat(), 6) : "N/A");

    Serial.print("Longitude:   ");
    Serial.println(gps.location.isValid() ? String(gps.location.lng(), 6) : "N/A");

    Serial.print("Altitude:    ");
    if (gps.altitude.isValid()) { Serial.print(gps.altitude.meters(), 2); Serial.println(" m"); }
    else Serial.println("N/A");

    Serial.print("Speed:       ");
    if (gps.speed.isValid()) { Serial.print(gps.speed.kmph(), 2); Serial.println(" km/h"); }
    else Serial.println("N/A");

    Serial.print("Course:      ");
    if (gps.course.isValid()) { Serial.print(gps.course.deg(), 2); Serial.println(" deg"); }
    else Serial.println("N/A");

    Serial.print("Satellites:  ");
    Serial.println(gps.satellites.isValid() ? String(gps.satellites.value()) : "N/A");

    Serial.print("HDOP:        ");
    Serial.println(gps.hdop.isValid() ? String(gps.hdop.hdop(), 2) : "N/A");

    Serial.print("Date:        ");
    if (gps.date.isValid())
        Serial.printf("%04d-%02d-%02d\n", gps.date.year(), gps.date.month(), gps.date.day());
    else
        Serial.println("N/A");

    Serial.print("Time (UTC):  ");
    if (gps.time.isValid())
        Serial.printf("%02d:%02d:%02d.%02d\n", gps.time.hour(), gps.time.minute(), gps.time.second(), gps.time.centisecond());
    else
        Serial.println("N/A");

    Serial.print("Chars in:    "); Serial.println(gps.charsProcessed());
    Serial.print("Sentences:   "); Serial.println(gps.sentencesWithFix());
    Serial.print("Checksum err:"); Serial.println(gps.failedChecksum());
    Serial.println("==============================");
}

void handleWifi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, reconnecting...");
        initWifi();
    }

    HTTPClient http;
    String url = String(SERVER_URL) +
                 "?bid=" + String(BOARD_ID) +
                 "&lat=" + String(gps.location.lat(), 6) +
                 "&lon=" + String(gps.location.lng(), 6);

    http.begin(url);
    int httpCode = http.GET();
    Serial.printf("HTTP %d\n", httpCode);
    http.end();
}

// ── Main ─────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Starting up...");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    initGPS();
    initWifi();
    Serial.println("Ready. Press button to report GPS.");
}

void loop() {
    while (SerialGPS.available()) {
        gps.encode(SerialGPS.read());
    }

    if (millis() - lastDebugMs > 5000) {
        lastDebugMs = millis();
        Serial.printf("[status] fix: %s | sats: %lu | wifi: %s\n",
            gps.location.isValid() ? "YES" : "waiting...",
            gps.satellites.isValid() ? gps.satellites.value() : 0,
            WiFi.status() == WL_CONNECTED ? "connected" : "disconnected"
        );
    }

    bool currentState = digitalRead(BUTTON_PIN);
    bool buttonPressed = (currentState == LOW && lastButtonState == HIGH);
    bool timerFired = (millis() - lastSendMs > 5000);

    if (buttonPressed || timerFired) {
        lastSendMs = millis();
        handleWifi();
        if (buttonPressed) delay(50);
    }
    lastButtonState = currentState;
}

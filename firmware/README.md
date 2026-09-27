# Firmware

ESP32/Arduino firmware that streams GPS fixes to the trackows backend (`POST
/api/events`) over Wi-Fi.

## Supported boards

| PlatformIO environment | Board                             | GPS source                      |
| ---------------------- | --------------------------------- | ------------------------------- |
| `lilygo-t-a7670g`      | LILYGO T-A7670G                   | External UART GPS on GPIO 22/21 |
| `lilygo-t-sim7000g`    | LILYGO T-SIM7000G (classic ESP32) | SIM7000G built-in GNSS          |

The T-SIM7000G modem uses GPIO 26/27 for its UART, GPIO 4 for PWRKEY, and GPIO
25 for DTR. Its GNSS antenna power is enabled through SIM7000G GPIO 48.

## Runtime

`HttpService` sends requests through a `Network` (`network.hpp`), an interface
with `connected()`, `macAddress()`, `ip()`, and `post()`. `WifiNetwork`
(`network/wifi.hpp`) implements it with `esp_http_client` and the ESP-IDF
certificate bundle; `init()` joins Wi-Fi and syncs time over NTP. A cellular
transport can replace it by implementing the same interface.

`GPS` and `HttpService` have lifecycle methods returning `Result<bool>`, and
each has its own event emitter (names in `events.hpp`):

```cpp
WifiNetwork network;
HttpService http(network, SERVER_URL);

gps.events().on(GpsEvent::Fix, [](void* data) {
    const auto& fix = *static_cast<GPSFix*>(data);
    http.events().emit(HttpEvent::PostLocation, HttpPostLocation{fix});
});
http.events().on(HttpEvent::Response, [](void* data) {
    const auto& response = *static_cast<HttpResponse*>(data);
    // response.ok, response.active, response.intersects
});

auto networkReady = network.init(WIFI_SSID, WIFI_PASS);
auto httpStarted = http.start();
auto gpsStarted = gps.start();
```

Each POST is the fix JSON plus `mac` (eFuse MAC, used as the board id) and `ip`.
On a `2xx` response from an active board, the T-A7670G pulses GPIO 32 for 1 s
when outside every fence, and the T-SIM7000G blinks its GPIO 12 LED once (two
quick blinks when inside a fence). Inactive boards (`active: false`, the
default) get no feedback.

GPS parsing, event dispatch, and HTTP run in persistent FreeRTOS tasks. Typed,
statically allocated FreeRTOS queues connect them, and HTTP requests are sent
sequentially so a slow request does not block GPS collection. The project
vendors an ESP32 adaptation of the task and queue wrappers from
[FreeRTOS-Cpp](https://github.com/jonenz/FreeRTOS-Cpp).

## Configuration

Create `firmware/.env`:

```dotenv
WIFI_SSID=your-network
WIFI_PASS=your-password
SERVER_URL=https://your-server/api/events
```

Install dependencies:

```sh
uv sync
```

## Building

`make` targets default to `ENV=lilygo-t-sim7000g` (plain `pio run` defaults to
`lilygo-t-a7670g`):

```sh
make build                       # T-SIM7000G
make build ENV=lilygo-t-a7670g   # T-A7670G
make build-all                   # both environments
make upload monitor              # flash and open the serial monitor
```

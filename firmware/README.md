# Firmware

ESP32/Arduino firmware that streams GPS fixes to an HTTP backend over Wi-Fi.

## Supported boards

| PlatformIO environment | Board | GPS source |
| --- | --- | --- |
| `lilygo-t-a7670g` | LILYGO T-A7670G | External UART GPS on GPIO 22/21 |
| `lilygo-t-sim7000g` | LILYGO T-SIM7000G (classic ESP32) | SIM7000G built-in GNSS |

The T-SIM7000G modem uses GPIO 26/27 for its UART, GPIO 4 for PWRKEY, and
GPIO 25 for DTR. Its GNSS antenna power is enabled through SIM7000G GPIO 48.

## Runtime

`GPS` and `Wifi` expose `start()` and `stop()` futures. Event handlers are
registered before startup:

```cpp
gps.onError(handleGpsError);
wifi.onError(handleWifiError);
gps.onFix([](const GPSFix& fix) {
    wifi.request("POST", SERVER_URL, fix.toJson());
});

auto wifiStarted = wifi.start();
auto gpsStarted = gps.start();
```

GPS parsing/polling and HTTP run on separate threads. HTTP requests enter a
bounded queue and are sent sequentially, so a slow request does not block GPS
collection.

## Configuration

Create `firmware/.env`:

```dotenv
WIFI_SSID=your-network
WIFI_PASS=your-password
SERVER_URL=http://your-server/api/event
```

Install dependencies:

```sh
uv sync
```

## Building

The A7670G environment is the default:

```sh
make build
```

Build specifically for the T-SIM7000G:

```sh
make build ENV=lilygo-t-sim7000g
```

Build both environments:

```sh
make build-all
```

Upload and monitor:

```sh
make upload ENV=lilygo-t-sim7000g
make monitor PORT=/dev/ttyUSB0
```

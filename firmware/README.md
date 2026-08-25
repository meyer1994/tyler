# Firmware

ESP32/Arduino firmware that streams GPS fixes to an HTTP backend over Wi-Fi.

## Supported boards

| PlatformIO environment | Board                             | GPS source                      |
| ---------------------- | --------------------------------- | ------------------------------- |
| `lilygo-t-a7670g`      | LILYGO T-A7670G                   | External UART GPS on GPIO 22/21 |
| `lilygo-t-sim7000g`    | LILYGO T-SIM7000G (classic ESP32) | SIM7000G built-in GNSS          |

The T-SIM7000G modem uses GPIO 26/27 for its UART, GPIO 4 for PWRKEY, and
GPIO 25 for DTR. Its GNSS antenna power is enabled through SIM7000G GPIO 48.

## Runtime

`GPS` and `Wifi` expose synchronous lifecycle methods returning `Result<bool>`.
Requests are queued immediately, while state and runtime failures are delivered
through callbacks:

```cpp
gps.onError(handleGpsError);
gps.onState(handleGpsState);
gps.onFixStateChange(handleFixState);
wifi.onError(handleWifiError);
static const WifiHeaders headers{
    {"Content-Type", "application/json"},
};

gps.onFix([](const GPSFix& fix) {
    auto queued = wifi.request("POST", SERVER_URL, fix.toJson(), headers);
});

auto wifiStarted = wifi.start();
auto gpsStarted = gps.start();
```

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

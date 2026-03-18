# Firmware

FreeRTOS-based ESP32 firmware for GPS tracking. Connects to WiFi and acquires a
GPS fix concurrently, then sends the location to the server via HTTP GET.

## Architecture

The firmware uses a task-based architecture where WiFi connection and GPS
acquisition happen in parallel. `SetupTask` is the orchestrator — it launches
both worker tasks and waits for their results via `Future<T>`.

### Task layout

```
setup()
  └── App::start()
        └── SetupTask (core 1)
              ├── GpsTask (core 0)   ← registers serial callback, exits
              └── WifiTask (core 1)  ← registers WiFi event, exits
```

Worker tasks exit immediately after registering their callbacks. Results are
delivered asynchronously via `Future<T>` and collected by `SetupTask`.

### Sequence diagram

```
setup()        SetupTask       GpsTask          WifiTask        Hardware
  │               │               │                │                │
  │  app.start()  │               │                │                │
  │──────────────>│               │                │                │
  │               │  _gps.start() │                │                │
  │               │──────────────>│                │                │
  │               │               │ onReceive(cb)  │                │
  │               │               │────────────────────────────────>│  (UART2)
  │               │               │               │                 │
  │               │               │<── run() returns, task deleted ─┘
  │               │               │                │
  │               │  _wifi.start()│                │
  │               │───────────────────────────────>│
  │               │               │                │ WiFi.onEvent(cb)
  │               │               │                │ WiFi.begin()
  │               │               │                │
  │               │               │                │<── run() returns, task deleted
  │               │               │                │
  │               │               │         [bytes arrive on UART2]
  │               │               │<── onReceive() fires ──────────>│
  │               │               │  gps.encode()  │                │
  │               │               │  location.isValid() == true     │
  │               │               │  result.set(fix)                │
  │               │               │                │                │
  │               │  [DHCP lease assigned]         │                │
  │               │               │   GOT_IP event fires            │
  │               │               │                │ result.set({true})
  │               │               │                │                │
  │    GpsFix fix = _gps.result.get(60s)           │                │
  │               │<──────────────│                │                │
  │               │               │                │                │
  │    WifiCtx ctx = _wifi.result.get(15s)         │                │
  │               │<──────────────────────────────>│                │
  │               │               │                │                │
  │               │  HTTPClient::GET(url)          │                │
  │               │────────────────────────────────────────────────>│  (TCP)
  │               │  Serial.printf("HTTP %d\n", code)               │
  │               │               │                │                │
```

### Flow diagram

```
                     ┌─────────────────────────────────────────┐
                     │               SetupTask                 │
                     │                                         │
  ┌──────────┐       │  ┌──────────┐         ┌──────────────┐  │
  │  setup() │──────>│  │ GpsTask  │         │  WifiTask    │  │
  └──────────┘       │  │ core 0   │         │  core 1      │  │
                     │  │          │         │              │  │
                     │  │onReceive │         │ WiFi.onEvent │  │
                     │  │callback  │         │ callback     │  │
                     │  └────┬─────┘         └──────┬───────┘  │
                     │       │                      │          │
                     │  [UART bytes]          [DHCP lease]     │
                     │       │                      │          │
                     │       ▼                      ▼          │
                     │  Future<GpsFix>      Future<WifiCtx>    │
                     │       │                      │          │
                     │       └──────────┬───────────┘          │
                     │                  │                      │
                     │           get() blocks                  │
                     │           (with timeout)                │
                     │                  │                      │
                     │         ┌────────┴────────┐             │
                     │         │  both ready?    │             │
                     │         └────────┬────────┘             │
                     │            yes   │   no (timeout)       │
                     │                  ▼        ▼             │
                     │           HTTP GET     log error        │
                     └─────────────────────────────────────────┘
```

## Timeout behaviour

| Condition          | Timeout | Result                                   |
| ------------------ | ------- | ---------------------------------------- |
| No GPS fix         | 60 s    | `[setup] GPS timeout`                    |
| WiFi not reachable | 15 s    | `[setup] WiFi timeout`                   |
| HTTP error         | —       | `[setup] HTTP <code>` (negative = error) |

## `Future<T>` semantics

Single-use. One `set()`, one `get()`. `get()` blocks until `set()` is called or
the timeout expires (returns zero-value struct on timeout). `ready()` consumes
the token — do not call it before `get()`.

## Building

```sh
pio run -t upload
pio device monitor -b 115200
```

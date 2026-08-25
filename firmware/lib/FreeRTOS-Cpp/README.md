# FreeRTOS-Cpp ESP32 adaptation

Vendored from [`jonenz/FreeRTOS-Cpp`](https://github.com/jonenz/FreeRTOS-Cpp)
at commit `15409dad07f1f119fafceef722a6b85dc830bd52`.

Only the task and queue wrappers used by this firmware are included. The headers
are adapted for ESP32 Arduino by using ESP-IDF's `freertos/...` include paths,
using `xTaskGetTickCount()` directly instead of the incompatible generic kernel
wrapper, and disabling APIs absent from this ESP-IDF FreeRTOS version.

The upstream MIT license is preserved in `LICENSE` and in each source header.

.PHONY: build build-all upload monitor

ENV ?= lilygo-t-a7670g
PORT ?= /dev/ttyACM0

build:
	uv run --env-file .env platformio run -e $(ENV)

build-all:
	uv run --env-file .env platformio run \
		-e lilygo-t-a7670g \
		-e lilygo-t-sim7000g

upload:
	pkill screen || true
	uv run --env-file .env platformio run -e $(ENV) --target upload

monitor:
	screen -L $(PORT) 115200

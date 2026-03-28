.PHONY: build upload monitor


build:
	pnpx dotenv-cli -e .env -- platformio run

upload:
	pkill screen || true
	pnpx dotenv-cli -e .env -- platformio run --target upload

monitor:
	screen -L /dev/ttyACM0 115200
# trackows

GPS tracking system with an ESP32 tracker, a Nuxt dashboard, and PostgreSQL/PostGIS.

## Project structure

- `app/` — Nuxt 4 web app, authentication, API, tRPC routes, database schema, and migrations
- `firmware/` — PlatformIO firmware for the LilyGO T-A7670G and T-SIM7000G ESP32 boards
- `.github/workflows/` — CI configuration

## App

```sh
cd app
pnpm install
cp .env.example .env
# Set DATABASE_URL and authentication values in .env
docker compose up -d postgres
pnpm run db:migrate
pnpm run dev
```

The dashboard is available at <http://localhost:3000>.

Stop the local PostgreSQL/PostGIS database with:

```sh
cd app
docker compose down
```

Useful checks:

```sh
pnpm run lint
pnpm run typecheck
pnpm run build
```

## Firmware

Set `WIFI_SSID`, `WIFI_PASS`, and `SERVER_URL` (ending in `/api/events`) in
`firmware/.env`, then run:

```sh
cd firmware
make build
make upload
make monitor
```

The firmware joins Wi-Fi and continuously posts GPS fixes, tagged with the
board's MAC address and IP. The server upserts the board, stores each event,
checks it against geofences, and replies whether the board is active and
inside a fence. Active boards signal the result locally, and boards start
inactive until toggled on the dashboard's Boards page.

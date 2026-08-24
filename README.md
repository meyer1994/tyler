# trackows

GPS tracking system with an ESP32 tracker, a Nuxt dashboard, and PostgreSQL/PostGIS.

## Project structure

- `app/` — Nuxt 4 web app, authentication, API, tRPC routes, database schema, and migrations
- `firmware/` — PlatformIO firmware for the LilyGO T-A7670 ESP32 board
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

Configure the server and cellular settings in `firmware/.env`, then run:

```sh
cd firmware
make build
make upload
make monitor
```

The firmware acquires a GPS fix, connects over cellular, posts telemetry to the configured server path, and then enters deep sleep.

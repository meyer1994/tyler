# AGENTS.md — trackows

GPS tracking: ESP32 posts fixes → Nuxt/PostGIS stores + geofence-intersects →
Better Auth dashboard (tRPC) maps/fences.

Monorepo: `app/` (Nuxt 4 + API), `firmware/` (PlatformIO ESP32),
`.github/workflows/`.

Env: `app/.env` → `DATABASE_URL`, `BETTER_AUTH_URL`, `BETTER_AUTH_SECRET`.
Firmware: `WIFI_SSID`, `WIFI_PASS`, `SERVER_URL` (build flags via
`platformio.ini`).

---

## APIs

### Device ingest (unauthenticated)

`POST /api/events` → `app/server/api/events.post.ts`

Body (JSON):
```
{ lat, lon, alt?, sat?, hdop?, speed?, heading?, ts?, mac?, ip? }  // ts = ISO datetime, mac = AA:BB:CC:DD:EE:FF, ip = IPv4/IPv6
```
- upserts `boards` (id = boardId; sets `last_seen_at`, keeps previous `ip` if
  none sent)
- inserts `events` (`boardId` = uppercased `mac`, falls back to `"BOARD"`),
  PostGIS `point` generated from lon/lat
- `ST_Intersects` vs `fences`; links via `rel_fences_events`
- `200` → `{ ok: true, intersects: boolean }` (no fence hit → `intersects:
  false`)

Firmware posts same shape (`GPSFix::toJson` + `mac` from eFuse + `ip` from
`Network::ip()`, added in `HttpService`); `SERVER_URL` should be full URL
ending in `/api/events` (CI still uses `/api/event` — mismatch).

### tRPC — `POST|GET /api/trpc/*` → `app/server/api/trpc/[trpc].ts`

Router: `app/server/trpc/index.ts`. Client: `$trpc` / `useTRPC()` → `/api/trpc`.

| Procedure       | Auth    | I/O                                                                                                                                                         |
| --------------- | ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ping`          | public  | → `"pong"`                                                                                                                                                  |
| `health`        | public  | → `{ status, timestamp }`                                                                                                                                   |
| `boards.list`   | session | → `{ id, name, ip, lastSeenAt, createdAt, updatedAt, eventsCount }[]` (last seen first, never-seen last) |
| `events.list`   | session | in `{ after?, before? }` ISO datetimes (optional; after≤before); → GeoJSON `FeatureCollection<Point>` (≤1000, newest first; props = event row sans `point`) |
| `fences.list`   | session | → GeoJSON `FeatureCollection<Polygon>` + props `{ id, area, createdAt, updatedAt, eventsCount }`                                                            |
| `fences.create` | session | in GeoJSON Feature `{ type:'Feature', geometry: Polygon }`; → void                                                                                          |
| `fences.update` | session | in `{ id, type:'Feature', geometry: Polygon }`; → void                                                                                                      |
| `fences.delete` | session | in `{ id }`; → void                                                                                                                                         |

`protectedProcedure` → 401 if no session. Context: `{ event, db, auth, session
}` (`app/server/lib/trpc.ts`).

### Better Auth — `/api/auth/*` → `app/server/api/auth/[...all].ts`

Email/password (`requireEmailVerification: false`). Client plugin provides
`signIn` / `signUp` / `signOut` / `useSession`. Tables: `user`, `session`,
`account`, `verification` (`server/db/auth.ts`).

### DB tables (domain)

| Table               | Key columns                                                                                          |
| ------------------- | ---------------------------------------------------------------------------------------------------- |
| `boards`            | id (uppercased MAC = events.board_id, no FK), name, ip, last_seen_at, created_at, updated_at         |
| `events`            | id, board_id, lat, lon, alt, speed, heading, hdop, sat, ts, point(generated), created_at, updated_at |
| `fences`            | id, polygon(4326), area(generated ST_Area), created_at, updated_at                                   |
| `rel_fences_events` | id, fence_id, event_id, created_at, updated_at                                                       |

Drizzle: `server/db/schema.ts`. Migrate: `pnpm run db:migrate` (PostGIS via
`docker compose` — `postgis/postgis:18-3.6-alpine`).

### Pages (Nuxt)

| Route                                                        | File                                |
| ------------------------------------------------------------ | ----------------------------------- |
| `/`                                                          | `app/app/pages/index.vue`           |
| `/signin`                                                    | `app/app/pages/signin.vue`          |
| `/dash`                                                      | `app/app/pages/dash.vue` + children |
| `/dash/maps`, `/dash/mapsgl`, `/dash/events`, `/dash/fences` | under `pages/dash/`                 |

---

## Trees

```
.
├── AGENTS.md
├── README.md
├── .github/workflows/build.yml    # app: lint/typecheck/build; firmware: both PIO envs
├── .vscode/
├── app/
│   ├── package.json               # pnpm; scripts: dev, build, lint, typecheck, db:*, auth:generate
│   ├── nuxt.config.ts             # node-server; Nuxt UI, maplibre, echarts, vueuse
│   ├── auth.config.ts             # Better Auth CLI schema gen
│   ├── drizzle.config.ts
│   ├── docker-compose.yml         # app + postgres/postgis
│   ├── Dockerfile
│   ├── gen-spiral.ts
│   ├── public/
│   ├── shared/                    # env.d.ts, wrangler.d.ts
│   ├── app/                       # Nuxt app source (srcDir)
│   │   ├── app.vue
│   │   ├── assets/css/main.css
│   │   ├── components/
│   │   │   ├── AsyncFetch.vue
│   │   │   ├── EventsTimeSlider.vue
│   │   │   ├── GraphLine.client.vue
│   │   │   ├── MapGL.client.vue
│   │   │   ├── MapGLDrawControl.client.vue
│   │   │   └── NavHeader.vue
│   │   ├── pages/
│   │   │   ├── index.vue
│   │   │   ├── signin.vue
│   │   │   ├── dash.vue
│   │   │   └── dash/{index,maps,mapsgl,events,fences}.vue
│   │   └── plugins/{auth,trpc,maplibre.client}.ts
│   └── server/
│       ├── api/
│       │   ├── events.post.ts
│       │   ├── auth/[...all].ts
│       │   └── trpc/[trpc].ts
│       ├── trpc/{index,boards,events,fences}.ts
│       ├── lib/{auth,drizzle,trpc}.ts
│       └── db/
│           ├── schema.ts          # events, fences, rel_fences_events + re-exports auth
│           ├── auth.ts            # Better Auth tables (generated)
│           └── migrations/
└── firmware/
    ├── Makefile                   # ENV=lilygo-t-sim7000g; build|build-all|upload|monitor
    ├── platformio.ini             # envs: lilygo-t-a7670g (default), lilygo-t-sim7000g
    ├── pyproject.toml             # uv + PlatformIO
    ├── src/
    │   ├── main.cpp               # Wi‑Fi + HTTP + GPS FreeRTOS wiring
    │   ├── events.{hpp,cpp}       # GPSFix JSON, event name constants
    │   ├── emitter.hpp
    │   ├── http.hpp               # queued POST of fixes via Network&
    │   ├── network.hpp            # Network interface: connected/macAddress/ip/post
    │   ├── network/wifi.hpp       # WifiNetwork: init(ssid, pass), esp_http_client
    │   ├── cellular.hpp           # TinyGSM (not wired in main)
    │   ├── gps.hpp
    │   ├── gps/{external,sim7000g}.hpp
    │   └── result.hpp
    └── lib/FreeRTOS-Cpp/src/FreeRTOS/{Queue,Task}.hpp
```

---

## Firmware notes

- Boards: `TRACKOWS_BOARD_A7670G` (UART GPS) / `TRACKOWS_BOARD_SIM7000G` (modem
  GNSS).
- Live path: `WifiNetwork::init` → `HttpService` builds JSON (`mac` from
  `Network::macAddress()`, eFuse MAC) → `Network::post` → parse
  `{ok,intersects}`. Swap transports by implementing `Network` (e.g. a future
  `CellularNetwork` over TinyGSM). Cellular header exists unused.
- Event bus names: `gps.fix`, `http.post_location`, `http.response`,
  `http.error` (see `events.hpp`).

## Commands

```sh
# app
cd app && pnpm install && docker compose up -d postgres && pnpm run db:migrate && pnpm run dev
# firmware
cd firmware && make build && make upload && make monitor
```

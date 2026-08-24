# trackows app

Nuxt 4 dashboard and API for GPS tracking. The app uses Vue, Nuxt UI, Better Auth, tRPC, Drizzle ORM, and PostgreSQL/PostGIS.

## Requirements

- Node.js 22+
- pnpm 10+
- Docker (for local PostgreSQL/PostGIS)

## Local development

```sh
cd app
pnpm install
cp .env.example .env
```

Set the database and authentication values in `.env`. For the local database,
use:

```env
DATABASE_URL=postgresql://postgres@localhost:5432/postgres
```

Start PostgreSQL/PostGIS, apply migrations, and run Nuxt:

```sh
docker compose up -d postgres
pnpm run db:migrate
pnpm run dev
```

Open <http://localhost:3000>.

Stop the database with:

```sh
docker compose down
```

## Useful commands

```sh
pnpm run dev          # Nuxt development server
pnpm run lint         # ESLint
pnpm run typecheck    # TypeScript checks
pnpm run build        # Production build
pnpm run preview      # Preview production build
pnpm run db:migrate   # Apply migrations
pnpm run db:generate  # Generate Drizzle migrations
pnpm run auth:generate # Regenerate Better Auth schema
```

## Application structure

- `app/` — Nuxt pages, components, plugins, and client-side UI
- `app/plugins/auth.ts` — Better Auth `$auth` Nuxt plugin and session handling
- `app/plugins/trpc.ts` — typed tRPC client
- `server/api/auth/[...all].ts` — Better Auth catch-all endpoint
- `server/api/event.post.ts` — GPS event ingestion endpoint
- `server/trpc/` — protected events and geofence procedures
- `server/db/schema.ts` — PostgreSQL/PostGIS schema
- `server/db/migrations/` — Drizzle migrations
- `public/` — static assets

## Authentication

The UI uses the `$auth` plugin exposed by `app/plugins/auth.ts`:

```ts
const { $auth } = useNuxtApp()
const { data: session } = $auth.useSession()

await $auth.signIn.email({ email, password })
await $auth.signOut()
```

There is no global auth middleware. API procedures that require a session use
protected tRPC procedures.

## GPS event ingestion

Devices send JSON telemetry to `POST /api/event`:

```json
{
  "lat": -27.5949,
  "lon": -48.5011,
  "alt": 20,
  "speed": 8,
  "heading": 180,
  "hdop": 0.9,
  "sat": 8,
  "ts": "2026-01-01T12:00:00Z"
}
```

Events are stored in PostgreSQL and associated with any geofences they
intersect.

## Docker

Build and run the app with its database:

```sh
docker compose up --build
```

The container applies migrations before starting the Nuxt server.

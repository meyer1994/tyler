#!/usr/bin/env -S pnpm exec tsx
/**
 * Generates PostgreSQL INSERT statements for a spiral of tracking events.
 *
 * Usage:
 *   pnpm exec tsx gen-spiral.ts | psql "$DATABASE_URL"
 *
 * Or, using the local .env file:
 *   pnpm exec dotenv -e .env -- sh -c 'pnpm exec tsx gen-spiral.ts | psql "$DATABASE_URL"'
 *
 * Options:
 *   --lat, --lon     Center coordinates (default: -27.5949, -48.5011)
 *   --boardId        Board ID for events (default: default)
 *   --points         Number of events (default: 50)
 *   --turns          Spiral turns (default: 5)
 *   --radius         Max radius in degrees (default: 0.01 ≈ 1.1km)
 *   --interval       Seconds between events (default: 60)
 *   --separate, -s   One INSERT per event instead of a single statement
 */

type Options = {
  lat: number
  lon: number
  boardId: string
  points: number
  turns: number
  radius: number
  interval: number
  separate: boolean
}

type Point = {
  lat: number
  lon: number
}

function parseArgs(): Options {
  const args = process.argv.slice(2)

  const get = (key: string, alt?: string): string | undefined => {
    const index = args.indexOf(key)
    if (index >= 0 && args[index + 1]) return args[index + 1]
    if (alt) return get(alt)
    return undefined
  }

  const options: Options = {
    lat: Number.parseFloat(get('--lat', '-l') ?? '-27.5949'),
    lon: Number.parseFloat(get('--lon', '-o') ?? '-48.5011'),
    boardId: get('--boardId', '--board') ?? 'default',
    points: Number.parseInt(get('--points', '-p') ?? '50', 10),
    turns: Number.parseFloat(get('--turns', '-t') ?? '5'),
    radius: Number.parseFloat(get('--radius', '-r') ?? '0.01'),
    interval: Number.parseFloat(get('--interval', '-i') ?? '60'),
    separate: args.includes('--separate') || args.includes('-s'),
  }

  if (!Number.isFinite(options.lat) || !Number.isFinite(options.lon)) {
    throw new Error('lat and lon must be numbers')
  }
  if (!Number.isInteger(options.points) || options.points < 1) {
    throw new Error('points must be a positive integer')
  }
  if (!Number.isFinite(options.turns) || !Number.isFinite(options.radius) || options.radius < 0) {
    throw new Error('turns and radius must be valid numbers')
  }
  if (!Number.isFinite(options.interval) || options.interval < 0) {
    throw new Error('interval must be a non-negative number')
  }

  return options
}

function spiralPoints(
  centerLat: number,
  centerLon: number,
  numPoints: number,
  turns: number,
  radiusDeg: number,
): Point[] {
  const points: Point[] = []
  const latRad = (centerLat * Math.PI) / 180
  const cosLat = Math.cos(latRad)

  for (let i = 0; i < numPoints; i++) {
    const t = numPoints > 1 ? i / (numPoints - 1) : 1
    const theta = 2 * Math.PI * turns * t
    const radius = radiusDeg * t

    points.push({
      lat: centerLat + radius * Math.cos(theta),
      lon: centerLon + (radius * Math.sin(theta)) / cosLat,
    })
  }

  return points
}

function sqlString(value: string): string {
  return `'${value.replace(/'/g, '\'\'')}'`
}

function sqlNumber(value: number, digits = 8): string {
  return value.toFixed(digits)
}

function eventRow(point: Point, index: number, options: Options): string {
  const age = sqlNumber((options.points - index - 1) * options.interval, 3)
  const altitude = sqlNumber(20 + Math.sin(index / 4) * 3, 2)
  const speed = sqlNumber(8 + Math.abs(Math.sin(index / 3)) * 12, 2)
  const heading = sqlNumber((index * 360 * options.turns) / Math.max(options.points, 1) % 360, 2)
  const hdop = sqlNumber(0.8 + Math.abs(Math.sin(index / 5)) * 0.8, 2)
  const satellites = 7 + (index % 6)
  const timestamp = `CURRENT_TIMESTAMP - INTERVAL '${age} seconds'`
  const id = `md5(random()::text || clock_timestamp()::text || ${index})`

  return `(${id}, ${sqlString(options.boardId)}, ${sqlNumber(point.lat)}, ${sqlNumber(point.lon)}, ${altitude}, ${speed}, ${heading}, ${hdop}, ${satellites}, ${timestamp}, ${timestamp}, ${timestamp})`
}

function main() {
  const options = parseArgs()
  const points = spiralPoints(
    options.lat,
    options.lon,
    options.points,
    options.turns,
    options.radius,
  )
  const rows = points.map((point, index) => eventRow(point, index, options))
  console.log('\\set ON_ERROR_STOP on')
  console.log('BEGIN;')
  if (options.separate) {
    for (const row of rows) {
      console.log(`INSERT INTO events (id, board_id, lat, lon, alt, speed, heading, hdop, sat, ts, created_at, updated_at) VALUES ${row};`)
    }
  }
  else {
    console.log('INSERT INTO events (id, board_id, lat, lon, alt, speed, heading, hdop, sat, ts, created_at, updated_at) VALUES')
    console.log(`${rows.join(',\n')};`)
  }
  console.log('COMMIT;')
}

main()

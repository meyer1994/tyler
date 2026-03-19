#!/usr/bin/env -S pnpm exec tsx
/**
 * Generates SQL INSERT statements for a spiral of geocoordinates.
 * Run the output in SQLite: pnpm spiral | sqlite3 your.db
 *
 * Usage:
 *   pnpm spiral [--lat <lat>] [--lon <lon>] [--boardId <id>] [options]
 *
 * Default center: [-27.5949, -48.5011] (lat, lon)
 *
 * Options:
 *   --lat, --lon     Center coordinates (default: -27.5949, -48.5011)
 *   --boardId        Board ID for events (default: default)
 *   --points         Number of points (default: 50)
 *   --turns          Spiral turns (default: 5)
 *   --radius         Max radius in degrees (default: 0.01 ≈ 1.1km)
 *   --separate, -s   One INSERT per row (default: single multi-row INSERT)
 */

function parseArgs(): {
  lat: number
  lon: number
  boardId: string
  points: number
  turns: number
  radius: number
  separate: boolean
} {
  const args = process.argv.slice(2)
  const get = (key: string, alt?: string): string | undefined => {
    const i = args.indexOf(key)
    if (i >= 0 && args[i + 1]) return args[i + 1]
    if (alt) return get(alt)
    return undefined
  }

  const lat = parseFloat(get('--lat', '-l') ?? '-27.5949')
  const lon = parseFloat(get('--lon', '-o') ?? '-48.5011')
  const boardId = get('--boardId', '--board') ?? 'default'
  const points = parseInt(get('--points', '-p') ?? '50', 10)
  const turns = parseFloat(get('--turns', '-t') ?? '5')
  const radius = parseFloat(get('--radius', '-r') ?? '0.01')
  const separate = args.includes('--separate') || args.includes('-s')

  return { lat, lon, boardId, points, turns, radius, separate }
}

function spiralPoints(
  centerLat: number,
  centerLon: number,
  numPoints: number,
  turns: number,
  radiusDeg: number,
): Array<{ lat: number, lon: number }> {
  const points: Array<{ lat: number, lon: number }> = []
  const latRad = (centerLat * Math.PI) / 180
  const cosLat = Math.cos(latRad)

  for (let i = 0; i < numPoints; i++) {
    const t = numPoints > 1 ? i / (numPoints - 1) : 1
    const theta = 2 * Math.PI * turns * t
    const r = radiusDeg * t

    const dlat = r * Math.cos(theta)
    const dlon = (r * Math.sin(theta)) / cosLat

    points.push({
      lat: centerLat + dlat,
      lon: centerLon + dlon,
    })
  }

  return points
}

function main() {
  const { lat, lon, boardId, points, turns, radius, separate } = parseArgs()
  const coords = spiralPoints(lat, lon, points, turns, radius)

  const escape = (s: string) => s.replace(/'/g, '\'\'')
  const rows = coords.map(
    ({ lat: l, lon: o }) =>
      `(lower(hex(randomblob(16))), '${escape(boardId)}', ${l}, ${o}, datetime('now'))`,
  )

  if (separate) {
    for (const row of rows) {
      console.log(
        `INSERT INTO events (id, board_id, lat, lon, created_at) VALUES ${row};`,
      )
    }
  }
  else {
    console.log(`INSERT INTO events (id, board_id, lat, lon, created_at) VALUES`)
    console.log(rows.join(',\n') + ';')
  }
}

main()

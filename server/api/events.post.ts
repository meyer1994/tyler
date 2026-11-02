import { sql } from 'drizzle-orm'
import * as z from 'zod'
import { boards, events, fences, relFencesEvents } from '../db/schema'
import { db } from '../lib/drizzle'

const query = z.object({
  lat: z.number(),
  lon: z.number(),
  alt: z.number().nullish(),
  sat: z.number().nullish(),
  hdop: z.number().nullish(),
  speed: z.number().nullish(),
  heading: z.number().nullish(),
  ts: z.iso.datetime().nullish(),
  mac: z.string().regex(/^(?:[0-9A-F]{2}:){5}[0-9A-F]{2}$/i).nullish(),
  ip: z.union([z.ipv4(), z.ipv6()]).nullish(),
})

export default defineEventHandler(async (event) => {
  const body = await readValidatedBody(event, query.parse)
  const boardId = body.mac?.toUpperCase() ?? 'BOARD'

  const [board] = await db
    .insert(boards)
    .values({
      id: boardId,
      ip: body.ip,
      lastSeenAt: sql`CURRENT_TIMESTAMP`,
    })
    .onConflictDoUpdate({
      target: boards.id,
      set: {
        ip: sql`COALESCE(excluded.ip, ${boards.ip})`,
        lastSeenAt: sql`CURRENT_TIMESTAMP`,
        updatedAt: sql`CURRENT_TIMESTAMP`,
      },
    })
    .returning({ active: boards.active })
  const active = board?.active ?? false

  const [item] = await db
    .insert(events)
    .values({
      lat: body.lat,
      lon: body.lon,
      alt: body.alt,
      speed: body.speed,
      heading: body.heading,
      hdop: body.hdop,
      sat: body.sat,
      ts: body.ts ? new Date(body.ts) : null,
      boardId,
    })
    .returning({
      id: events.id,
      point: events.point,
    })
  if (!item) return

  const intersects = await db
    .select({ id: fences.id })
    .from(fences)
    .where(
      sql`ST_Intersects(${fences.polygon}, ST_SetSRID(ST_MakePoint(${item.point.x}, ${item.point.y}), 4326))`)
  console.log('[events.post] intersects', intersects)

  if (intersects.length < 1) return { ok: true, active, intersects: false }

  const result = await db
    .insert(relFencesEvents)
    .values(intersects.map(f => ({ eventId: item.id, fenceId: f.id })))
    .returning({ id: relFencesEvents.id })

  return { ok: true, active, intersects: result.length > 0 }
})

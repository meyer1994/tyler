import * as z from 'zod'
import { events } from '../db/schema'
import { serverDrizzle } from '../lib/drizzle'

const query = z.object({
  lat: z.number(),
  lon: z.number(),
  alt: z.number().nullish(),
  sat: z.number().nullish(),
  hdop: z.number().nullish(),
  speed: z.number().nullish(),
  heading: z.number().nullish(),
  ts: z.iso.datetime().nullish(),
})

export default defineEventHandler(async (event) => {
  const body = await readValidatedBody(event, query.parse)
  console.log(body)

  const task = async () => {
    const db = serverDrizzle(event)
    await db.insert(events).values({
      lat: body.lat,
      lon: body.lon,
      alt: body.alt,
      speed: body.speed,
      heading: body.heading,
      hdop: body.hdop,
      sat: body.sat,
      ts: body.ts ? new Date(body.ts) : null,
      boardId: 'BOARD',
    })
  }

  event.waitUntil(task())
  return { ok: true }
})

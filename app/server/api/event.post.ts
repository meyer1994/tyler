import * as z from 'zod'
import { events } from '../db/schema'
import { serverDrizzle } from '../lib/drizzle'

const query = z.object({
  lat: z.number(),
  lon: z.number(),
  alt: z.number().optional(),
  sat: z.number().optional(),
})

export default defineEventHandler(async (event) => {
  const body = await readValidatedBody(event, query.parse)
  console.log(body)

  const task = async () => {
    const db = serverDrizzle(event)
    await db.insert(events).values({
      lat: body.lat,
      lon: body.lon,
      boardId: 'BOARD',
    })
  }

  event.waitUntil(task())
  return { ok: true }
})

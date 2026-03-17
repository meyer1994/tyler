import * as z from 'zod'
import { serverDrizzle } from '../lib/drizzle'
import { events } from '../db/schema'

const query = z.object({
  bid: z.string().min(1),
  lat: z.coerce.number(),
  lon: z.coerce.number(),
})

export default defineEventHandler(async (event) => {
  const params = await getValidatedQuery(event, query.parse)
  const db = serverDrizzle(event)

  await db.insert(events).values({
    boardId: params.bid,
    lat: params.lat,
    lon: params.lon,
  })

  return { ok: true }
})

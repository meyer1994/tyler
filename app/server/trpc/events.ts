import type { FeatureCollection, Point } from 'geojson'
import { desc } from 'drizzle-orm'
import { events } from '~~/server/db/schema'
import { createTRPCRouter, protectedProcedure } from '../lib/trpc'

export const eventsRouter = createTRPCRouter({
  list: protectedProcedure
    .query(async ({ ctx }) => {
      const data = await ctx.db
        .select()
        .from(events)
        .orderBy(desc(events.createdAt))
        .limit(1000)
        .execute()

      type Item = Omit<typeof data[number], 'point'>

      const coll: FeatureCollection<Point, Item> = {
        type: 'FeatureCollection',
        features: data.map(e => ({
          id: e.id,
          type: 'Feature',
          geometry: { type: 'Point', coordinates: [e.lon, e.lat] },
          properties: { ...e, point: undefined },
        })),
      }

      return coll
    }),
})

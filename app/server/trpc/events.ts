import type { FeatureCollection, Point } from 'geojson'
import { and, desc, gte, lte } from 'drizzle-orm'
import * as z from 'zod'
import { events } from '~~/server/db/schema'
import { createTRPCRouter, protectedProcedure } from '../lib/trpc'

const listInput = z.object({
  after: z.iso.datetime().optional(),
  before: z.iso.datetime().optional(),
}).refine(
  input => !input.after || !input.before || input.after <= input.before,
  { message: '`after` must be before or equal to `before`' },
).optional()

export const eventsRouter = createTRPCRouter({
  list: protectedProcedure
    .input(listInput)
    .query(async ({ ctx, input }) => {
      const filters = [
        input?.after
          ? gte(events.createdAt, new Date(input.after))
          : undefined,
        input?.before
          ? lte(events.createdAt, new Date(input.before))
          : undefined,
      ].filter(filter => filter !== undefined)

      const data = await ctx.db
        .select()
        .from(events)
        .where(filters.length > 0 ? and(...filters) : undefined)
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

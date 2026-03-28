import { desc, sql } from 'drizzle-orm'
import z from 'zod'
import { events, fences } from '~~/server/db/schema'
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

      const coll: GeoJSON.FeatureCollection<GeoJSON.Point, Item> = {
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

  polygons: protectedProcedure
    .query(async ({ ctx }) => {
      const data = await ctx.db
        .select()
        .from(fences)
        .orderBy(desc(fences.createdAt))
        .execute()

      type Item = Omit<typeof data[number], 'polygon'>

      const coll: GeoJSON.FeatureCollection<GeoJSON.Polygon, Item> = {
        type: 'FeatureCollection',
        features: data.map(e => ({
          id: e.id,
          type: 'Feature',
          geometry: e.polygon,
          properties: { ...e, polygon: undefined },
        })),
      }

      return coll
    }),

  createFence: protectedProcedure
    .input(z.object({
      type: z.literal('Polygon'),
      coordinates: z.array(z.array(z.tuple([z.number(), z.number()]))),
    }))
    .mutation(async ({ ctx, input }) => {
      return await ctx.db
        .insert(fences)
        .values({ polygon: sql`ST_GeomFromGeoJSON(${JSON.stringify(input)})` })
        .returning()
    }),
})

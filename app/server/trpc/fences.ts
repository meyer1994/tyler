import { countDistinct, desc, eq } from 'drizzle-orm'
import z from 'zod'
import { fences, relFencesEvents } from '~~/server/db/schema'
import { createTRPCRouter, protectedProcedure } from '../lib/trpc'

export const fencesRouter = createTRPCRouter({
  list: protectedProcedure
    .query(async ({ ctx }) => {
      const data = await ctx.db
        .select({
          id: fences.id,
          polygon: fences.polygon,
          area: fences.area,
          createdAt: fences.createdAt,
          updatedAt: fences.updatedAt,
          eventsCount: countDistinct(relFencesEvents.eventId),
        })
        .from(fences)
        .leftJoin(relFencesEvents, eq(fences.id, relFencesEvents.fenceId))
        .groupBy(fences.id)
        .orderBy(desc(fences.createdAt))

      type Properties = Omit<typeof data[number], 'polygon'>

      const features: GeoJSON.FeatureCollection<GeoJSON.Polygon, Properties> = {
        type: 'FeatureCollection',
        features: data.map(f => ({
          id: f.id,
          type: 'Feature',
          geometry: f.polygon,
          properties: {
            id: f.id,
            area: f.area,
            createdAt: f.createdAt,
            updatedAt: f.updatedAt,
            eventsCount: f.eventsCount,
          },
        })),
      }

      return features
    }),

  create: protectedProcedure
    .input(z.object({
      type: z.literal('Feature'),
      geometry: z.object({
        type: z.literal('Polygon'),
        coordinates: z.array(z.array(z.array(z.number()))),
      }),
    }))
    .mutation(async ({ ctx, input }) => {
      await ctx.db
        .insert(fences)
        .values({ polygon: input.geometry })
    }),

  update: protectedProcedure
    .input(z.object({
      id: z.string(),
      type: z.literal('Feature'),
      geometry: z.object({
        type: z.literal('Polygon'),
        coordinates: z.array(z.array(z.array(z.number()))),
      }),
    }))
    .mutation(async ({ ctx, input }) => {
      await ctx.db
        .update(fences)
        .set({ polygon: input.geometry })
        .where(eq(fences.id, input.id))
    }),

  delete: protectedProcedure
    .input(z.object({ id: z.string() }))
    .mutation(async ({ ctx, input }) => {
      await ctx.db
        .delete(fences)
        .where(eq(fences.id, input.id))
    }),
})

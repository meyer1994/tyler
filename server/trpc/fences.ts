import { desc, eq } from 'drizzle-orm'
import type { Feature, FeatureCollection, Polygon } from 'geojson'
import z from 'zod'
import { fences } from '~~/server/db/schema'
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
        })
        .from(fences)
        .orderBy(desc(fences.createdAt))

      type Properties = Omit<typeof data[number], 'polygon'>

      const features: FeatureCollection<Polygon, Properties> = {
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
          },
        })),
      }

      return features
    }),

  get: protectedProcedure
    .input(z.object({ id: z.string() }))
    .query(async ({ ctx, input }) => {
      const [fence] = await ctx.db
        .select({
          id: fences.id,
          polygon: fences.polygon,
          area: fences.area,
          createdAt: fences.createdAt,
          updatedAt: fences.updatedAt,
        })
        .from(fences)
        .where(eq(fences.id, input.id))
        .limit(1)

      if (!fence) {
        throw createError({ status: 404, message: 'Fence not found' })
      }

      const feature: Feature<Polygon, {
        id: string
        area: number
        createdAt: Date
        updatedAt: Date
      }> = {
        type: 'Feature',
        id: fence.id,
        geometry: fence.polygon,
        properties: {
          id: fence.id,
          area: fence.area,
          createdAt: fence.createdAt,
          updatedAt: fence.updatedAt,
        },
      }

      return feature
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

import { desc, eq, sql } from 'drizzle-orm'
import z from 'zod'
import { fences } from '~~/server/db/schema'
import { createTRPCRouter, protectedProcedure } from '../lib/trpc'

export const fencesRouter = createTRPCRouter({
  list: protectedProcedure
    .query(async ({ ctx }) => {
      const data = await ctx.db
        .select({
          id: fences.id,
          type: sql`'Feature'`,
          geometry: fences.polygon,
          properties: sql`json_build_object(
            'id', ${fences.id}, 
            'createdAt', ${fences.createdAt}, 
            'updatedAt', ${fences.updatedAt})
          `,
        })
        .from(fences)
        .orderBy(desc(fences.createdAt))
        .execute()

      type Props = { id: string, createdAt: string, updatedAt: string }
      type Coll = GeoJSON.FeatureCollection<GeoJSON.Polygon, Props>
      return { type: 'FeatureCollection', features: data } as Coll
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
      const [data] = await ctx.db
        .insert(fences)
        .values({ polygon: input.geometry })
        .returning({
          id: fences.id,
          type: sql`'Feature'`,
          geometry: fences.polygon,
          properties: sql`json_build_object(
            'id', ${fences.id}, 
            'createdAt', ${fences.createdAt}, 
            'updatedAt', ${fences.updatedAt})
          `,
        })

      if (!data) throw createError({ statusCode: 400 })

      type Props = { id: string, createdAt: string, updatedAt: string }
      return data as GeoJSON.Feature<GeoJSON.Polygon, Props>
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
      const [data] = await ctx.db
        .update(fences)
        .set({ polygon: input.geometry })
        .where(eq(fences.id, input.id))
        .returning({
          id: fences.id,
          type: sql`'Feature'`,
          geometry: fences.polygon,
          properties: sql`json_build_object(
            'id', ${fences.id}, 
            'createdAt', ${fences.createdAt}, 
            'updatedAt', ${fences.updatedAt})
          `,
        })

      if (!data) throw createError({ statusCode: 400 })

      type Props = { id: string, createdAt: string, updatedAt: string }
      return data as GeoJSON.Feature<GeoJSON.Polygon, Props>
    }),

  delete: protectedProcedure
    .input(z.object({ id: z.string() }))
    .mutation(async ({ ctx, input }) => {
      const [data] = await ctx.db
        .delete(fences)
        .where(eq(fences.id, input.id))
        .returning({
          id: fences.id,
          type: sql`'Feature'`,
          geometry: fences.polygon,
          properties: sql`json_build_object(
            'id', ${fences.id}, 
            'createdAt', ${fences.createdAt}, 
            'updatedAt', ${fences.updatedAt})
          `,
        })

      if (!data) throw createError({ statusCode: 400 })

      type Props = { id: string, createdAt: string, updatedAt: string }
      return data as GeoJSON.Feature<GeoJSON.Polygon, Props>
    }),
})

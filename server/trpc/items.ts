import { and, desc, eq } from 'drizzle-orm'
import * as z from 'zod'
import { TItems } from '~~/server/db/schema'
import { createTRPCRouter, protectedProcedure } from '../lib/trpc'

export const itemsRouter = createTRPCRouter({
  create: protectedProcedure
    .input(z.object({ name: z.string() }))
    .mutation(async ({ input, ctx }) => {
      return await ctx.db
        .insert(TItems)
        .values({ name: input.name, userId: ctx.session.user.id })
        .returning()
        .get()
    }),

  delete: protectedProcedure
    .input(z.object({ id: z.string() }))
    .mutation(async ({ input, ctx }) => {
      await ctx.db
        .delete(TItems)
        .where(
          and(
            eq(TItems.id, input.id),
            eq(TItems.userId, ctx.session.user.id)))
    }),

  update: protectedProcedure
    .input(z.object({ id: z.string(), name: z.string() }))
    .mutation(async ({ input, ctx }) => {
      return await ctx.db
        .update(TItems)
        .set({ name: input.name })
        .where(
          and(
            eq(TItems.id, input.id),
            eq(TItems.userId, ctx.session.user.id)))
        .returning()
        .get()
    }),

  list: protectedProcedure
    .query(async ({ ctx }) => {
      return await ctx.db
        .select()
        .from(TItems)
        .where(eq(TItems.userId, ctx.session.user.id))
        .orderBy(desc(TItems.createdAt))
        .all()
    }),
})

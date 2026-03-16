import { and, desc, eq } from 'drizzle-orm'
import { z } from 'zod'
import { TTasks } from '../db/schema'
import { createTRPCRouter, protectedProcedure } from '../lib/trpc'

export const queueRouter = createTRPCRouter({
  send: protectedProcedure
    .input(z.object({
      message: z.string().min(1),
    }))
    .mutation(async ({ input, ctx }) => {
      const task = await ctx.db
        .insert(TTasks)
        .values({
          status: 'SENDING',
          message: input.message,
          userId: ctx.session.user.id,
        })
        .returning()
        .get()

      ctx.event.waitUntil(ctx.queue.send({ id: task.id, message: input.message }))
      return task
    }),

  list: protectedProcedure
    .query(async ({ ctx }) => {
      return await ctx.db
        .select()
        .from(TTasks)
        .where(eq(TTasks.userId, ctx.session.user.id))
        .orderBy(desc(TTasks.createdAt))
    }),

  delete: protectedProcedure
    .input(z.object({ id: z.string() }))
    .mutation(async ({ input, ctx }) => {
      return await ctx.db
        .delete(TTasks)
        .where(
          and(
            eq(TTasks.id, input.id),
            eq(TTasks.userId, ctx.session.user.id)))
        .returning()
        .get()
    }),
})

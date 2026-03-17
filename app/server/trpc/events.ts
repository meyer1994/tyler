import { desc } from 'drizzle-orm'
import { events } from '~~/server/db/schema'
import { createTRPCRouter, protectedProcedure } from '../lib/trpc'

export const eventsRouter = createTRPCRouter({
  list: protectedProcedure
    .query(async ({ ctx }) => {
      return await ctx.db
        .select()
        .from(events)
        .orderBy(desc(events.createdAt))
        .limit(1000)
        .all()
    }),
})

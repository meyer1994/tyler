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
        // .where(
        //   and(
        //     isNotNull(events.boardId),
        //     isNotNull(events.lat),
        //     isNotNull(events.lon)))
        .limit(100)
        .all()
    }),
})

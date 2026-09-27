import { count, eq, sql } from 'drizzle-orm'
import { boards, events } from '~~/server/db/schema'
import { createTRPCRouter, protectedProcedure } from '../lib/trpc'

export const boardsRouter = createTRPCRouter({
  list: protectedProcedure
    .query(async ({ ctx }) => {
      return await ctx.db
        .select({
          id: boards.id,
          name: boards.name,
          ip: boards.ip,
          lastSeenAt: boards.lastSeenAt,
          createdAt: boards.createdAt,
          updatedAt: boards.updatedAt,
          eventsCount: count(events.id),
        })
        .from(boards)
        .leftJoin(events, eq(boards.id, events.boardId))
        .groupBy(boards.id)
        .orderBy(sql`${boards.lastSeenAt} DESC NULLS LAST`)
    }),
})

import { avg, count, desc, gt } from 'drizzle-orm'
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

  stats: protectedProcedure
    .query(async ({ ctx }) => {
      const now = new Date()
      const ago01h = new Date(now.getTime() - 60 * 60 * 1000)
      const ago24h = new Date(now.getTime() - 24 * 60 * 60 * 1000)

      const q1h = ctx.db
        .select({ count: count() })
        .from(events)
        .where(gt(events.createdAt, ago01h))
        .get()

      const q24h = ctx.db
        .select({ count: count() })
        .from(events)
        .where(gt(events.createdAt, ago24h))
        .get()

      const qAvgs = ctx.db
        .select({ avgHdop: avg(events.hdop), avgSats: avg(events.sat) })
        .from(events)
        .get()

      const [r1h, r24h, rAvgs] = await Promise.all([q1h, q24h, qAvgs])

      return {
        last1hCount: r1h?.count ?? -1,
        last24hCount: r24h?.count ?? -1,
        avgHdop: rAvgs?.avgHdop ? Number(rAvgs.avgHdop).toFixed(2) : -1,
        avgSats: rAvgs?.avgSats ? Number(rAvgs.avgSats).toFixed(2) : -1,
      } as {
        last1hCount: number
        last24hCount: number
        avgHdop: number
        avgSats: number
      }
    }),
})

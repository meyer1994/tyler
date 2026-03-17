import { z } from 'zod'
import { createTRPCRouter, protectedProcedure } from '../lib/trpc'

export const vectorRouter = createTRPCRouter({
  search: protectedProcedure
    .input(z.object({
      query: z.string().min(1),
      prefix: z.string().optional(),
    }))
    .query(async ({ input, ctx }) => {
      const prefix = `${ctx.session.user.id}/`
      return await ctx.vector.search(input.query, { prefix })
    }),
})

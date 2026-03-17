import * as z from 'zod'
import { createTRPCRouter, protectedProcedure } from '../lib/trpc'
import { onFileUpload } from '../tasks/onFileUpload'

export const filesRouter = createTRPCRouter({
  create: protectedProcedure
    .input(z
      .instanceof(FormData)
      .transform(fd => Object.fromEntries(fd.entries()))
      .pipe(
        z.object({
          file: z.instanceof(File)
            .refine(f => f.size > 0)
            .refine(f => f.size <= 10 * 1024 * 1024) // 10MB
            .refine(f => f.type === 'text/plain' || f.type === 'text/markdown')
            .refine(f => f.name.trim().length > 0),
        })))
    .mutation(async ({ input, ctx }) => {
      const key = `${ctx.session.user.id}/${input.file.name}`
      await ctx.storage.put(key, input.file)
      ctx.event.waitUntil(onFileUpload(key, ctx))
    }),

  delete: protectedProcedure
    .input(z.object({ key: z.string() }))
    .mutation(async ({ input, ctx }) => {
      if (!input.key.startsWith(`${ctx.session.user.id}/`))
        throw createError({ status: 403, message: 'Unauthorized' })

      await Promise.all([
        ctx.storage.del(input.key),
        ctx.vector.del(input.key),
      ])
    }),

  list: protectedProcedure
    .query(async ({ ctx }) => {
      const items = await ctx.storage.list(`${ctx.session.user.id}/`)

      return await Promise.all(items.map(async item => ({
        ...item,
        url: await ctx.storage.presign(item.key),
      })))
    }),
})

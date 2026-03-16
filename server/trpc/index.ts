import type { inferRouterInputs, inferRouterOutputs } from '@trpc/server'
import { filesRouter } from '~~/server/trpc/files'
import { itemsRouter } from '~~/server/trpc/items'
import { queueRouter } from '~~/server/trpc/queue'
import { vectorRouter } from '~~/server/trpc/vector'
import { baseProcedure, createTRPCRouter } from '../lib/trpc'

export const appRouter = createTRPCRouter({
  ping: baseProcedure
    .query(() => 'pong'),
  health: baseProcedure
    .query(() => ({ status: 'ok', timestamp: new Date().toISOString() })),

  items: itemsRouter,
  files: filesRouter,
  vector: vectorRouter,
  queue: queueRouter,
})

// export type definition of API
export type AppRouter = typeof appRouter
export type AppRouterInputs = inferRouterInputs<AppRouter>
export type AppRouterOutputs = inferRouterOutputs<AppRouter>

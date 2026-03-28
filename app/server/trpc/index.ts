import type { inferRouterInputs, inferRouterOutputs } from '@trpc/server'
import { eventsRouter } from '~~/server/trpc/events'
import { fencesRouter } from '~~/server/trpc/fences'
import { baseProcedure, createTRPCRouter } from '../lib/trpc'

export const appRouter = createTRPCRouter({
  ping: baseProcedure
    .query(() => 'pong'),
  health: baseProcedure
    .query(() => ({ status: 'ok', timestamp: new Date().toISOString() })),

  events: eventsRouter,
  fences: fencesRouter,
})

// export type definition of API
export type AppRouter = typeof appRouter
export type AppRouterInputs = inferRouterInputs<AppRouter>
export type AppRouterOutputs = inferRouterOutputs<AppRouter>

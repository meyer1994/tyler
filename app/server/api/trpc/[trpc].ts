import type { EventHandlerRequest, H3Event } from 'h3'
import { createTRPCNuxtHandler } from 'trpc-nuxt/server'
import { createTRPCContext } from '../../lib/trpc'
import { appRouter } from '../../trpc'

export default createTRPCNuxtHandler({
  endpoint: '/api/trpc',
  router: appRouter,
  createContext: async event => createTRPCContext(event as H3Event<EventHandlerRequest>),
})

import { createTRPCNuxtHandler } from 'trpc-nuxt/server'
import { createTRPCContext } from '../../lib/trpc'
import { appRouter } from '../../trpc'

export default createTRPCNuxtHandler({
  endpoint: '/api/trpc',
  router: appRouter,
  createContext: createTRPCContext,
})

import { initTRPC, type AnyRouter } from '@trpc/server'
import type { BetterAuthClientOptions, InferSessionFromClient, InferUserFromClient } from 'better-auth'
import type { H3Event } from 'h3'
import { serverAuth } from './auth'
import { serverDrizzle } from './drizzle'
import { serverQueue } from './queue'
import { serverStorage } from './storage'
import { serverVector } from './vector'

type Session = {
  user: InferUserFromClient<BetterAuthClientOptions>
  session: InferSessionFromClient<BetterAuthClientOptions>
}

export type TRPCContext = {
  event: H3Event
  db: ReturnType<typeof serverDrizzle>
  storage: ReturnType<typeof serverStorage>
  vector: ReturnType<typeof serverVector>
  queue: ReturnType<typeof serverQueue>
  auth: ReturnType<typeof serverAuth>
  session: Session | null
}

export type TRPCContextProtected = TRPCContext & {
  session: Session
}

export const createTRPCContext = async (event: H3Event) => {
  /**
  * @see: https://trpc.io/docs/server/context
  */

  const auth = serverAuth(event)
  const db = serverDrizzle(event)
  const storage = serverStorage(event)
  const vector = serverVector(event)
  const queue = serverQueue(event)
  const session = await auth.api.getSession({ headers: event.headers })
  console.info('[server.trpc] user id', session?.user?.id)
  console.info('[server.trpc] session id', session?.session?.id)

  return {
    event,
    db,
    storage,
    vector,
    queue,
    auth,
    session,
  } satisfies TRPCContext
}

// Avoid exporting the entire t-object since it's not very descriptive. For
// instance, the use of a t variable is common in i18n libraries.
const t = initTRPC.context<TRPCContext>().create({
  /**
  * @see https://trpc.io/docs/server/data-transformers
  */
  // transformer: superjson,
})

const logger = t.middleware(async ({ next, ctx }) => {
  const path = ctx.event.path.split('?')[0]
  console.info(`[server.trpc] start ${path}`)
  const start = Date.now()

  const result = await next({ ctx })
  if (!result.ok) console.error('[server.trpc] error', result)

  const duration = Date.now() - start
  console.info(`[server.trpc] end ${path} - ${duration}ms`)
  return result
})

const isAuthenticated = t.middleware(async ({ next, ctx }) => {
  if (!ctx.session) throw createError({ status: 401, message: 'No session found' })
  if (!ctx.session.user) throw createError({ status: 401, message: 'No user found in session' })

  const path = ctx.event.path.split('?')[0]
  console.info(`[server.trpc] user ${ctx.session.user.id} authenticated for ${path}`)

  return next({ ctx } as { ctx: TRPCContextProtected })
})

// Base router and procedure helpers
export const createTRPCRouter = t.router
export const createCallerFactory = t.createCallerFactory
export const baseProcedure = t.procedure.use(logger)
export const protectedProcedure = t.procedure.use(logger).use(isAuthenticated)

/**
 * Creates an internal tRPC caller for SSR to bypass HTTP subrequests.
 * @see https://trpc.io/docs/server/server-side-calls
 */
export const serverTRPC = async <TRouter extends AnyRouter>(
  router: TRouter,
  event: H3Event,
) => {
  const context = await createTRPCContext(event)
  return createCallerFactory(router)(context)
}

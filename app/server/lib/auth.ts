import { betterAuth } from 'better-auth'
import { drizzleAdapter } from 'better-auth/adapters/drizzle'
import type { H3Event } from 'h3'
import * as schema from '../db/schema'
import { serverDrizzle } from './drizzle'

export const serverAuth = (event: H3Event) => {
  const db = serverDrizzle(event)

  const auth = betterAuth({
    database: drizzleAdapter(db, {
      provider: 'sqlite',
      // debugLogs: true,
      schema,
    }),

    advanced: {
      disableOriginCheck: import.meta.dev,
    },

    emailAndPassword: {
      enabled: true,
      requireEmailVerification: false,
    },

    plugins: [],

    trustedOrigins: [
      'http://localhost:3000',
      'http://localhost:8787',
      'https://*.meyer1994.workers.dev',
    ],
  })

  return auth
}

import { betterAuth } from 'better-auth'
import { drizzleAdapter } from 'better-auth/adapters/drizzle'
import * as schema from '../db/schema'

import { db } from './drizzle'

export const auth = betterAuth({
    database: drizzleAdapter(db, {
      schema,
      provider: 'sqlite',
      debugLogs: import.meta.dev,
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

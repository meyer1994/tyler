import { betterAuth } from 'better-auth'
import { drizzleAdapter } from 'better-auth/adapters/drizzle'
import * as schema from './server/db/schema'

export const auth = betterAuth({
  database: drizzleAdapter(schema, {
    provider: 'pg',
    schema,
  }),

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

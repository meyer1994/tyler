import { betterAuth } from 'better-auth'
import { drizzleAdapter } from 'better-auth/adapters/drizzle'
import { drizzle } from 'drizzle-orm/sql-js'
import { globSync } from 'node:fs'
import * as pathLib from 'node:path'
import * as schema from './server/db/schema'

const [firstMatch] = globSync('.wrangler/**/*D1DatabaseObject*/**/*.sqlite')
console.log('sqlite file', firstMatch)
const path = firstMatch ? `file:${pathLib.resolve(firstMatch)}` : ''
const db = drizzle(path, { schema })

export const auth = betterAuth({
  database: drizzleAdapter(db, {
    provider: 'sqlite',
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

import { betterAuth } from 'better-auth'
import { drizzleAdapter } from 'better-auth/adapters/drizzle'
import { stripe } from '@better-auth/stripe'
import { apiKey, magicLink, organization } from 'better-auth/plugins'
import { drizzle } from 'drizzle-orm/sql-js'
import { globSync } from 'node:fs'
import * as pathLib from 'node:path'
import Stripe from 'stripe'
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

  plugins: [
    apiKey(),
    organization(),
    stripe({
      stripeClient: new Stripe(process.env.STRIPE_SECRET_KEY!),
      stripeWebhookSecret: process.env.STRIPE_WEBHOOK_SECRET!,
      subscription: {
        enabled: true,
        plans: [
          { name: 'starter', priceId: process.env.STRIPE_PRICE_STARTER! },
          { name: 'pro', priceId: process.env.STRIPE_PRICE_PRO! },
        ],
      },
    }),
    magicLink({
      sendMagicLink: async () => {},
    }),
  ],

  trustedOrigins: [
    'http://localhost:3000',
    'http://localhost:8787',
    'https://*.meyer1994.workers.dev',
  ],
})

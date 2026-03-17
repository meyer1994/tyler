import { stripe } from '@better-auth/stripe'
import type { SecondaryStorage } from 'better-auth'
import { betterAuth } from 'better-auth'
import { drizzleAdapter } from 'better-auth/adapters/drizzle'
import { apiKey, magicLink, organization } from 'better-auth/plugins'
import type { H3Event } from 'h3'
import type Stripe from 'stripe'
import * as schema from '../db/schema'
import { serverDrizzle } from './drizzle'
import { serverEmail } from './email'
import { serverStripe } from './stripe'

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const serverAuthSecondaryStorage = (event: H3Event): SecondaryStorage => {
  const kv = event.context.cloudflare.env.KV as KVNamespace

  return {
    get: async (key: string) => {
      console.info('[server.auth.cache] get', key)
      const value = await kv.get(key)
      console.info('[server.auth.cache] get result', key, value ? 'HIT' : 'MISS')
      return value
    },
    set: async (key: string, value: string, ttl?: number) => {
      console.info('[server.auth.cache] set', key, { ttl })
      if (!ttl) {
        await kv.put(key, value)
        console.info('[server.auth.cache] set completed', key, 'no TTL')
        return
      }
      await kv.put(key, value, { expirationTtl: Math.max(ttl ?? 0, 60) })
      console.info('[server.auth.cache] set completed', key, 'with TTL', ttl)
    },
    delete: async (key: string) => {
      console.info('[server.auth.cache] delete', key)
      await kv.delete(key)
      console.info('[server.auth.cache] delete completed', key)
    },
  }
}

const createStripe = (client: Stripe, env: Env) => stripe({
  stripeClient: client,
  stripeWebhookSecret: env.STRIPE_WEBHOOK_SECRET,

  subscription: {
    enabled: true,
    plans: [
      {
        name: 'starter' as const,
        priceId: env.STRIPE_PRICE_STARTER,
      },
      {
        name: 'pro' as const,
        priceId: env.STRIPE_PRICE_PRO,
      },
    ],
  },
})

export const serverAuth = (event: H3Event) => {
  const env = event.context.cloudflare.env as unknown as Env

  const db = serverDrizzle(event)
  const mailgun = serverEmail(event)
  const stripe = serverStripe(event)

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

    plugins: [
      organization({
        async sendInvitationEmail(data, request) {
          const url = new URL(request?.url || 'http://localhost:3000')
          try {
            await mailgun.send({
              to: data.email,
              text: `You have been invited to join ${data.organization.name}. Click here to accept: ${url.origin}/invitation?id=${data.id}`,
            })
          }
          catch (error) {
            console.error('[server.auth.sendInvitationEmail] error', error)
            throw error
          }
        },
      }),
      apiKey(),
      createStripe(stripe, env),
      magicLink({
        sendMagicLink: async (email) => {
          try {
            await mailgun.send({
              to: email.email,
              text: `Click here to login: ${email.url}`,
            })
          }
          catch (error) {
            console.error('[server.auth.sendMagicLink] error', error)
            throw error
          }
        },
      }),
    ],

    trustedOrigins: [
      'http://localhost:3000',
      'http://localhost:8787',
      'https://*.meyer1994.workers.dev',
    ],
  })

  return auth
}

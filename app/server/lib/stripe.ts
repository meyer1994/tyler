import type { H3Event } from 'h3'
import Stripe from 'stripe'

export const serverStripe = (event: H3Event) => {
  const key = event.context.cloudflare.env.STRIPE_SECRET_KEY as string
  if (!key) throw new Error('STRIPE_SECRET_KEY not found')
  return new Stripe(key)
}

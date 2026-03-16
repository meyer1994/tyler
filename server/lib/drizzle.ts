import type { H3Event } from 'h3'

import { drizzle } from 'drizzle-orm/d1'
import * as schema from '../db/schema'

export const serverDrizzle = (event: H3Event) => {
  const db = event.context.cloudflare.env.DB as D1Database
  return drizzle(db, {
    schema,
    logger: {
      logQuery: (query, params) => {
        if (!import.meta.dev) return
        console.info(`[server.drizzle] query ${query} with params ${params}`)
      },
    },
  })
}

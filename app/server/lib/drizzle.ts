import type { H3Event } from 'h3'

import { drizzle } from 'drizzle-orm/node-postgres'
import * as schema from '../db/schema'

let db: ReturnType<typeof drizzle> | null = null

export const serverDrizzle = (event: H3Event) => {
  if (!db) db = drizzle(process.env.DATABASE_URL!, { schema })
  return db
}

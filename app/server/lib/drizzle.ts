import type { H3Event } from 'h3'

import { drizzle } from 'drizzle-orm/node-postgres'
import * as schema from '../db/schema'

let db: ReturnType<typeof drizzle> | null = null

class Logger {
  logQuery(query: string, params: any[]) {
    console.log(query, params)
  }
}

export const serverDrizzle = (event: H3Event) => {
  if (!db) db = drizzle(process.env.DATABASE_URL!, { schema, logger: new Logger() })
  return db
}

import { drizzle } from 'drizzle-orm/node-postgres'
import * as schema from '../db/schema'

class Logger {
  logQuery(query: string, params: any[]) {
    if (import.meta.dev) console.debug(query, params)
  }
}

export const db = drizzle(process.env.DATABASE_URL!, { 
  schema, 
  logger: new Logger(),
})

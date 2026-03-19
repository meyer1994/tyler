import { sql } from 'drizzle-orm'
import { index, integer, real, sqliteTable, text } from 'drizzle-orm/sqlite-core'

export * from './auth'

export const events = sqliteTable('events', {
  id: text('id')
    .primaryKey()
    .$defaultFn(() => crypto.randomUUID()),
  boardId: text('board_id')
    .notNull(),
  lat: real('lat')
    .notNull(),
  lon: real('lon')
    .notNull(),
  createdAt: integer('created_at', { mode: 'timestamp_ms' })
    .notNull()
    .default(sql`(unixepoch('subsecond') * 1000)`),
  updatedAt: integer('updated_at', { mode: 'timestamp_ms' })
    .notNull()
    .default(sql`(unixepoch('subsecond') * 1000)`)
    .$onUpdate(() => sql`(unixepoch('subsecond') * 1000)`),
},
table => [
  index('events_board_id_idx').on(table.boardId),
  index('events_created_at_idx').on(table.createdAt),
  index('events_lat_lon_idx').on(table.lat, table.lon),
])

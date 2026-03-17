import { sql } from 'drizzle-orm'
import { index, real, sqliteTable, text } from 'drizzle-orm/sqlite-core'

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
  createdAt: text('created_at')
    .notNull()
    .default(sql`CURRENT_TIMESTAMP`),
},
table => [
  index('events_board_id_idx').on(table.boardId),
  index('events_created_at_idx').on(table.createdAt),
])

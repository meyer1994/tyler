import { desc, sql } from 'drizzle-orm'
import { index, sqliteTable, text } from 'drizzle-orm/sqlite-core'
import * as auth from './auth'

export * from './auth'

export const TItems = sqliteTable('items', {
  id: text('id')
    .primaryKey()
    .$defaultFn(() => crypto.randomUUID()),
  userId: text('user_id')
    .notNull()
    .references(() => auth.user.id, { onDelete: 'cascade' }),
  name: text('name')
    .notNull(),
  createdAt: text('created_at')
    .notNull()
    .default(sql`CURRENT_TIMESTAMP`),
  updatedAt: text('updated_at')
    .notNull()
    .default(sql`CURRENT_TIMESTAMP`)
    .$onUpdateFn(() => sql`CURRENT_TIMESTAMP`),
},
table => [
  index('items_user_id_idx').on(table.userId),
  index('items_created_at_idx').on(desc(table.createdAt)),
  index('items_updated_at_idx').on(desc(table.updatedAt)),
])

export const TTasks = sqliteTable('tasks', {
  id: text('id')
    .primaryKey()
    .$defaultFn(() => crypto.randomUUID()),
  message: text('message')
    .notNull(),
  userId: text('user_id')
    .notNull()
    .references(() => auth.user.id, { onDelete: 'cascade' }),
  status: text('status', { enum: ['SENDING', 'PENDING', 'RUNNING', 'ERRORED', 'FINISHED'] })
    .notNull()
    .default('PENDING'),
  error: text('error'),
  createdAt: text('created_at')
    .notNull()
    .default(sql`CURRENT_TIMESTAMP`),
  updatedAt: text('updated_at')
    .notNull()
    .default(sql`CURRENT_TIMESTAMP`)
    .$onUpdateFn(() => sql`CURRENT_TIMESTAMP`),
},
table => [
  index('tasks_status_idx').on(table.status),
  index('tasks_user_id_idx').on(table.userId),
  index('tasks_created_at_idx').on(desc(table.createdAt)),
  index('tasks_updated_at_idx').on(desc(table.updatedAt)),
])

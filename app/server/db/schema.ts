import type { SQL } from 'drizzle-orm'
import { sql } from 'drizzle-orm'
import {
  geometry,
  index,
  integer,
  pgTable,
  real,
  text,
  timestamp,
} from 'drizzle-orm/pg-core'

export * from './auth'

export const events = pgTable('events',
  {
    id: text('id')
      .primaryKey()
      .$defaultFn(() => crypto.randomUUID()),
    boardId: text('board_id')
      .notNull(),

    lat: real('lat').notNull(),
    lon: real('lon').notNull(),

    alt: real('alt'),
    speed: real('speed'),
    heading: real('heading'),
    hdop: real('hdop'),
    sat: integer('sat'),
    ts: timestamp('ts', { withTimezone: true }),

    point: geometry('point', { type: 'point', mode: 'xy', srid: 4326 })
      .generatedAlwaysAs((): SQL => sql`ST_SetSRID(ST_MakePoint(${events.lon}, ${events.lat}), 4326)`),

    createdAt: timestamp('created_at', { withTimezone: true })
      .notNull()
      .default(sql`CURRENT_TIMESTAMP`),
    updatedAt: timestamp('updated_at', { withTimezone: true })
      .notNull()
      .default(sql`CURRENT_TIMESTAMP`)
      .$onUpdate(() => sql`CURRENT_TIMESTAMP`),
  },
  table => [
    index('events_board_id_idx').on(table.boardId),
    index('events_created_at_idx').on(table.createdAt),
    index('events_lat_lon_idx').on(table.lat, table.lon),
    index('events_point_idx').using('gist', table.point),
  ],
)

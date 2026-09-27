import type { Polygon } from 'geojson'
import type { SQL } from 'drizzle-orm'
import { relations, sql } from 'drizzle-orm'
import {
  customType,
  geometry,
  index,
  integer,
  pgTable,
  real,
  text,
  timestamp,
} from 'drizzle-orm/pg-core'

import { Geometry } from 'wkx'

export * from './auth'

// Adapted from:
// https://github.com/drizzle-team/drizzle-orm/discussions/2383#discussioncomment-11895854
const polygon = customType<{ data: Polygon, driverData: string }>({
  dataType() {
    return 'geometry(Polygon, 4326)'
  },
  toDriver(value: Polygon): SQL {
    return sql`ST_GeomFromGeoJSON(${JSON.stringify(value)})`
  },
  fromDriver(data: string): Polygon {
    const buffer = Buffer.from(data, 'hex')
    const geoJson = Geometry.parse(buffer).toGeoJSON()
    return geoJson as Polygon
  },
})

export const boards = pgTable('boards',
  {
    // Uppercased MAC address, matches `events.board_id`.
    id: text('id')
      .primaryKey(),

    name: text('name'),
    ip: text('ip'),
    lastSeenAt: timestamp('last_seen_at', { withTimezone: true }),

    createdAt: timestamp('created_at', { withTimezone: true })
      .notNull()
      .default(sql`CURRENT_TIMESTAMP`),
    updatedAt: timestamp('updated_at', { withTimezone: true })
      .notNull()
      .default(sql`CURRENT_TIMESTAMP`)
      .$onUpdate(() => sql`CURRENT_TIMESTAMP`),
  },
  table => [
    index('boards_last_seen_at_idx').on(table.lastSeenAt),
    index('boards_created_at_idx').on(table.createdAt),
    index('boards_updated_at_idx').on(table.updatedAt),
  ],
)

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
      .notNull()
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
    index('events_updated_at_idx').on(table.updatedAt),
    index('events_lat_lon_idx').on(table.lat, table.lon),
    index('events_lon_lat_idx').on(table.lon, table.lat),
    index('events_point_idx').using('gist', table.point),
  ],
)

export const fences = pgTable('fences',
  {
    id: text('id')
      .primaryKey()
      .$defaultFn(() => crypto.randomUUID()),

    polygon: polygon('polygon')
      .notNull(),
    area: real('area')
      .notNull()
      .generatedAlwaysAs((): SQL => sql`ST_Area(${fences.polygon}, true)`),

    createdAt: timestamp('created_at', { withTimezone: true })
      .notNull()
      .default(sql`CURRENT_TIMESTAMP`),
    updatedAt: timestamp('updated_at', { withTimezone: true })
      .notNull()
      .default(sql`CURRENT_TIMESTAMP`)
      .$onUpdate(() => sql`CURRENT_TIMESTAMP`),
  },
  table => [
    index('fences_created_at_idx').on(table.createdAt),
    index('fences_updated_at_idx').on(table.updatedAt),
    index('fences_polygon_idx').using('gist', table.polygon),
  ],
)

export const relFencesEvents = pgTable('rel_fences_events',
  {
    id: text('id')
      .primaryKey()
      .$defaultFn(() => crypto.randomUUID()),

    fenceId: text('fence_id')
      .references(() => fences.id, { onDelete: 'cascade', onUpdate: 'cascade' }),
    eventId: text('event_id')
      .references(() => events.id, { onDelete: 'cascade', onUpdate: 'cascade' }),

    createdAt: timestamp('created_at', { withTimezone: true })
      .notNull()
      .default(sql`CURRENT_TIMESTAMP`),
    updatedAt: timestamp('updated_at', { withTimezone: true })
      .notNull()
      .default(sql`CURRENT_TIMESTAMP`)
      .$onUpdate(() => sql`CURRENT_TIMESTAMP`),
  },
  table => [
    index('fences_events_fence_id_idx').on(table.fenceId),
    index('fences_events_event_id_idx').on(table.eventId),
    index('fences_events_fence_id_event_id_idx').on(table.fenceId, table.eventId),
    index('fences_events_event_id_fence_id_idx').on(table.eventId, table.fenceId),
    index('fences_events_created_at_idx').on(table.createdAt),
    index('fences_events_updated_at_idx').on(table.updatedAt),
  ],
)

export const fencesRelations = relations(fences, ({ many }) => ({
  events: many(relFencesEvents),
}))

export const boardsRelations = relations(boards, ({ many }) => ({
  events: many(events),
}))

export const eventsRelations = relations(events, ({ one, many }) => ({
  board: one(boards, {
    fields: [events.boardId],
    references: [boards.id],
  }),
  fences: many(relFencesEvents),
}))

export const fencesEventsRelations = relations(relFencesEvents, ({ one }) => ({
  fence: one(fences, {
    fields: [relFencesEvents.fenceId],
    references: [fences.id],
  }),
  event: one(events, {
    fields: [relFencesEvents.eventId],
    references: [events.id],
  }),
}))

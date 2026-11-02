import type { SQL } from 'drizzle-orm'
import { sql } from 'drizzle-orm'
import {
  customType,
  index,
  pgTable,
  real,
  text,
  timestamp,
} from 'drizzle-orm/pg-core'
import type { Polygon } from 'geojson'
import { uuidv7 } from 'uuidv7'

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

export const fences = pgTable('fences',
  {
    id: text('id')
      .primaryKey()
      .$defaultFn(() => uuidv7()),

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

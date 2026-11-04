import { desc } from 'drizzle-orm'
import { z } from 'zod'
import { fences } from '~~/server/db/schema'
import { db } from '~~/server/lib/drizzle'

const schemaPath = z.object({
  x: z.coerce.number(),
  y: z.coerce.number(),
  z: z.coerce.number(),
})

const schemaQuery = z.object({
  view: z.enum(['ndvi', 'rgb']).default('ndvi'),
  collection: z.string().default('sentinel-2-l2a'),
})

const templates = {
  rgb: {
    assets: ['visual'],
    rescale: [[0, 255]],
  },
  ndvi: {
    assets: ['nir', 'red'],
    expr: '(b1-b2)/(b1+b2)',
    rescale: [[-1, 1]],
    colormap: 'rdylgn',
  },
} as const

export default defineEventHandler(async (event) => {
  const { view, collection } = await getValidatedQuery(event, schemaQuery.parse)
  const { x, y, z } = await getValidatedRouterParams(event, schemaPath.parse)

  const config = useRuntimeConfig(event)
  const url = new URL(config.tiles)
  url.pathname = '/tiles'
  console.info('[server.tiles] url', url, 'view', view, 'collection', collection)

  const rows = await db
    .select({ geometry: fences.polygon })
    .from(fences)
    .orderBy(desc(fences.createdAt))
  console.info('[server.tiles] total rows', rows.length)

  return await $fetch(url.toString(), {
    method: 'POST',
    body: {
      x,
      y,
      z,
      fmt: 'png',
      geometries: rows.map(i => i.geometry),
      collection,
      ...templates[view],
    },
  })
})

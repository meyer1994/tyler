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
  const config = useRuntimeConfig(event)
  const { x, y, z } = await getValidatedRouterParams(event, schemaPath.parse)
  const { view } = await getValidatedQuery(event, schemaQuery.parse)

  const url = new URL(config.tiles)
  console.info('[server.tiles] url', url, 'view', view)

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
      ...templates[view],
    },
  })
})

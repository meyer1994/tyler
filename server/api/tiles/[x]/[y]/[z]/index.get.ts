import { desc } from 'drizzle-orm'
import { z } from 'zod'
import { fences } from '~~/server/db/schema'
import { db } from '~~/server/lib/drizzle'
import { Collections, View } from '~~/shared/types'

const schemaPath = z.object({
  x: z.coerce.number(),
  y: z.coerce.number(),
  z: z.coerce.number(),
})

type TileTemplate = {
  assets: string[]
  expr?: string
  rescale?: number[][]
  colormap?: string
}

const TEMPLATES: Record<Collections, Partial<Record<View, TileTemplate>>> = {
  'sentinel-2-l2a': {
    rgb: { assets: ['visual'], rescale: [[0, 255]] },
    ndvi: { assets: ['nir', 'red'], expr: '(b1-b2)/(b1+b2)', rescale: [[-1, 1]], colormap: 'rdylgn' },
  },
  'sentinel-2-c1-l2a': {
    rgb: { assets: ['visual'], rescale: [[0, 255]] },
    ndvi: { assets: ['nir', 'red'], expr: '(b1-b2)/(b1+b2)', rescale: [[-1, 1]], colormap: 'rdylgn' },
  },
  // 'landsat-c2-l2': {
  //   rgb: { assets: ['red', 'green', 'blue'], rescale: [[0, 65535], [0, 65535], [0, 65535]] },
  //   ndvi: { assets: ['nir08', 'red'], expr: '(b1-b2)/(b1+b2)', rescale: [[-1, 1]], colormap: 'rdylgn' },
  // },
  // 'naip': {
  //   // `image` is a 4-band (R, G, B, NIR) asset
  //   rgb: { assets: ['image'], rescale: [[0, 255]] },
  //   ndvi: { assets: ['image'], expr: '(b4-b1)/(b4+b1)', rescale: [[-1, 1]], colormap: 'rdylgn' },
  // },
  // 'sentinel-1-grd': {
  //   rgb: { assets: ['vv'], rescale: [[0, 1]] },
  // },
  // 'cop-dem-glo-30': {
  //   rgb: { assets: ['data'], rescale: [[0, 4000]], colormap: 'terrain' },
  // },
  // 'cop-dem-glo-90': {
  //   rgb: { assets: ['data'], rescale: [[0, 4000]], colormap: 'terrain' },
  // },
}

const schemaQuery = z
  .object({
    view: z.enum(View).default(View.NDVI),
    collection: z.enum(Collections).default(Collections.SENTINEL_2_L2A),
  })
  .refine(({ view, collection }) => !!TEMPLATES[collection][view])
  .transform(({ view, collection }) => ({
    view,
    collection,
    template: TEMPLATES[collection][view]!,
  }))

export default defineEventHandler(async (event) => {
  const { view, collection, template } = await getValidatedQuery(event, schemaQuery.parse)
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
      ...template,
    },
  })
})

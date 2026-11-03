import { desc } from 'drizzle-orm'
import { z } from 'zod'
import { fences } from '~~/server/db/schema'
import { db } from '~~/server/lib/drizzle'

const schemaPath = z.object({
  x: z.coerce.number(),
  y: z.coerce.number(),
  z: z.coerce.number(),
})

export default defineEventHandler(async (event) => {
  const { x, y, z } = await getValidatedRouterParams(event, schemaPath.parse)

  const url = new URL('http://localhost:8000/tiles')
  console.info('[server.tiles] url', url)

  const rows = await db
    .select({ geometry: fences.polygon })
    .from(fences)
    .orderBy(desc(fences.createdAt))
  console.info('[server.tiles] rows', rows)

  return await $fetch(url.toString(), {
    method: 'POST',
    body: {
      x: x,
      y: y,
      z: z,
      fmt: 'png',
      geometries: rows.map(i => i.geometry),
      assets: ['nir', 'red'],
      expr: '(b1-b2)/(b1+b2)',
      rescale: [[-1, 1]],
      colormap: 'rdylgn',
    },
  })
})

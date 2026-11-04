import { desc, eq } from 'drizzle-orm'
import { z } from 'zod'
import { fences } from '~~/server/db/schema'
import { db } from '~~/server/lib/drizzle'

const schemaPath = z.object({
  id: z.string(),
})

const schemaQuery = z.object({
  href: z.url(),
  view: z.enum(['ndvi', 'rgb']).default('ndvi'),
  collection: z.string().default('sentinel-2-l2a'),
})

export default defineEventHandler(async (event) => {
  const { href, view, collection } = await getValidatedQuery(event, schemaQuery.parse)
  const { id } = await getValidatedRouterParams(event, schemaPath.parse)

  const config = useRuntimeConfig(event)
  const url = new URL(config.tiles)
  url.pathname = '/image'
  console.info('[server.tiles] url', url, 'view', view, 'collection', collection)

  const [row] = await db
    .select({ geometry: fences.polygon })
    .from(fences)
    .where(eq(fences.id, id))
    .orderBy(desc(fences.createdAt))
    .limit(1)
  console.info('[server.tiles] total rows', row ? 1 : 0)

  if (!row) throw createError({
    status: 404,
    message: `Fence ${id} not found`,
  })

  return await $fetch(url.toString(), {
    method: 'POST',
    body: {
      href,
      geometry: row.geometry,
      collection,
    },
  })
})

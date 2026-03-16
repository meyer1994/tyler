import { eq } from 'drizzle-orm'
import type { DrizzleD1Database } from 'drizzle-orm/d1'
import { drizzle } from 'drizzle-orm/d1'
import * as schema from '../db/schema'
import { TTasks } from '../db/schema'

type QueueMessage = {
  attempts: number
  body: { message: string, id: string }
  timestamp: string
  id: string
}

type DB = DrizzleD1Database<typeof schema>

async function processMessage(db: DB, body: QueueMessage) {
  console.info('[server.queue] processing event id', body.id)
  console.info('[server.queue] processing task id', body.body.id)

  // Update status to RUNNING
  await db
    .update(TTasks)
    .set({ status: 'RUNNING' })
    .where(eq(TTasks.id, body.body.id))

  try {
    // TODO: Add your task processing logic here
    console.info('[server.queue] task message:', body.body.message)

    // Simulate some work
    await new Promise(resolve => setTimeout(resolve, 100))

    // Update status to FINISHED
    await db
      .update(TTasks)
      .set({ status: 'FINISHED' })
      .where(eq(TTasks.id, body.body.id))

    console.info('[server.queue] task finished', body.id)
  }
  catch (error) {
    const message = error instanceof Error ? error.message : 'Unknown error'
    console.error('[server.queue] task error', body.body.id, message)

    // Update status to ERRORED
    await db
      .update(TTasks)
      .set({ status: 'ERRORED', error: message })
      .where(eq(TTasks.id, body.body.id))
  }
}

export default defineNitroPlugin((nitro) => {
  nitro.hooks.hook('cloudflare:queue', async (event) => {
    console.info('[server.queue] queue event', { json: JSON.stringify(event, null, 2) })
    if (event.batch.queue !== 'flower') return

    const env = event.env as unknown as Env
    const db = drizzle(env.DB, { schema })

    const promises = (event.batch.messages as QueueMessage[])
      .map(async i => processMessage(db, i))
    await Promise.all(promises)
  })
})

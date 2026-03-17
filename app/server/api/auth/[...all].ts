import { serverAuth } from '../../lib/auth'

export default defineEventHandler(async (event) => {
  console.info(`[server.auth] handler method: ${event.method} path: ${event.path}`)
  const start = Date.now()

  const auth = serverAuth(event)
  const response = await auth.handler(toWebRequest(event))

  const duration = Date.now() - start
  console.info(`[server.auth] handler completed in ${duration}ms`)
  return response
})

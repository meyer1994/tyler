import { serverAuth } from '~~/server/lib/auth'

export default defineEventHandler(async (event) => {
  const auth = serverAuth(event)

  // Get API key from header
  const apiKey = getHeader(event, 'x-api-key')

  if (!apiKey) {
    throw createError({
      status: 401,
      message: 'Missing API key. Provide x-api-key header.',
    })
  }

  // Verify the API key
  const result = await auth.api.verifyApiKey({ body: { key: apiKey } })

  if (!result.valid) {
    throw createError({
      status: 401,
      message: 'Unauthorized',
    })
  }

  // Return success response with key info
  return {
    message: 'API key is valid!',
    timestamp: new Date().toISOString(),
  }
})

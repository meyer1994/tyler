declare module '#app' {
  interface PageMeta {
    auth?: boolean
  }
}

declare module 'vue-router' {
  interface RouteMeta {
    auth?: boolean
  }
}

export default defineNuxtRouteMiddleware(async (to, from) => {
  if (import.meta.client) {
    console.info(`[client.middleware.auth.global] from ${from.path} to ${to.path}`)
  }
  if (import.meta.server) {
    console.info(`[server.middleware.auth.global] from ${from.path} to ${to.path}`)
  }

  const { loggedIn, fetchSession } = useAuth()
  await fetchSession()

  // skip if not protected route
  if (!to.meta.auth) return
  // abort if not logged in (404)
  if (!loggedIn.value) return abortNavigation()
})

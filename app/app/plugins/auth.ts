import { createAuthClient } from 'better-auth/vue'

export default defineNuxtPlugin(() => {
  const url = useRequestURL()
  const headers = import.meta.server ? useRequestHeaders(['cookie']) : undefined

  const client = createAuthClient({
    baseURL: url.origin,
    fetchOptions: { headers },
  })

  type SessionData = NonNullable<Awaited<ReturnType<typeof client.getSession>>['data']>

  const useSession = () => {
    return useAsyncData<SessionData | null>('auth-session', async () => {
      const { data, error } = await client.getSession()
      if (error) throw createError(error)
      if (!data?.session || !data?.user) return null
      return data
    })
  }

  return {
    provide: {
      auth: {
        signIn: client.signIn,
        signUp: client.signUp,
        signOut: client.signOut,
        useSession,
      },
    },
  }
})

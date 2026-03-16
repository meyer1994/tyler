import { stripeClient } from '@better-auth/stripe/client'
import { apiKeyClient, magicLinkClient, organizationClient } from 'better-auth/client/plugins'
import { createAuthClient } from 'better-auth/vue'

export function useAuth() {
  const url = useRequestURL()
  const headers = useRequestHeaders()

  const client = createAuthClient({
    baseURL: url.origin,
    fetchOptions: { headers },
    plugins: [
      organizationClient(),
      apiKeyClient(),
      stripeClient({ subscription: true }),
      magicLinkClient(),
    ],
  })

  const { data, refresh } = useAsyncData('session', async () => {
    console.info('[client.auth] fetching session')
    const session = await client.getSession()
    return session.data
  })

  const user = computed(() => data.value?.user || null)
  const session = computed(() => data.value?.session || null)
  const loggedIn = computed(() => !!data.value?.user)

  const fetchSession = async () => {
    await refresh()
    return session.value
  }

  const signOut = async () => {
    await client.signOut()
    data.value = null
  }

  return {
    user,
    session,
    loggedIn,
    client: client as typeof client,
    signOut,
    fetchSession,
    signIn: client.signIn,
    signUp: client.signUp,
    apiKey: client.apiKey,
  }
}

type AuthAPI = ReturnType<typeof useAuth>['client']
const _item: AuthAPI = {} as AuthAPI
export const $Infer = _item.$Infer

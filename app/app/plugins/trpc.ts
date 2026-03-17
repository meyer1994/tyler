import { isNonJsonSerializable, loggerLink, splitLink } from '@trpc/client'
import { createTRPCNuxtClient, httpBatchLink, httpLink } from 'trpc-nuxt/client'
import type { AppRouter } from '~~/server/trpc'

/**
 * The useTRPC composable should not use a cached client instance. otherwise, it
 * will use the wrong headers, the ones from the first requests, resulting in
 * the client being unauthorized in case there were no cookies set yet
 */
export const useTRPC = () => {
  console.info('[client.trpc] useTRPC')

  const url = useRequestURL()
  url.pathname = '/api/trpc'
  console.info('[client.trpc] url', url.href)

  return createTRPCNuxtClient<AppRouter>({
    links: [
      loggerLink({ enabled: () => typeof window !== 'undefined' }),
      splitLink({
        // we need this split link to be able to send files, and FormData to the
        // trpc server. we cannot use send "multiple" FormData in a single request
        condition: op => isNonJsonSerializable(op.input),
        true: httpLink({ url: url.href }),
        false: httpBatchLink({ url: url.href }),
      }),
    ],
  })
}

export default defineNuxtPlugin(() => {
  return { provide: { trpc: useTRPC() } }
})

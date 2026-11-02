<!-- components/AsyncData.vue -->
<script setup lang="ts" generic="T">
import type { AsyncDataOptions } from '#app'

type FetchKey
  = | string
    | string[]
    | MaybeRef<string>
    | MaybeRef<string[]>
    | (() => string)
    | (() => string[])

const props = defineProps<{
  fetchKey: FetchKey
  handler: () => Promise<T>
  options?: AsyncDataOptions<unknown, T>
}>()

const key = computed<string>(() => {
  let key = props.fetchKey
  key = isRef(key) ? key.value : key
  key = typeof key === 'function' ? key() : key
  return Array.isArray(key) ? key.join(':') : key
})

const { data, status, error, refresh, clear } = useAsyncData<T>(key, async () => {
  return await props.handler()
})

type Slots = {
  data: typeof data.value
  status: typeof status.value
  error: typeof error.value
  refresh: typeof refresh
  clear: typeof clear
}

defineSlots<{ default(props: Slots): Slots }>()
</script>

<template>
  <slot
    :data="data"
    :status="status"
    :error="error"
    :refresh="refresh"
    :clear="clear"
  />
</template>

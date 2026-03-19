<!-- components/FormSubmit.vue -->
<script setup lang="ts" generic="T, P">
import type { AsyncDataOptions } from '#app'
import { ref } from 'vue'
import type { ZodError, ZodType } from 'zod'

const props = defineProps<{
  schema: ZodType<P>
  handler: (payload: P) => Promise<T>
  options?: AsyncDataOptions<unknown, T>
}>()

const validation = ref<ZodError | null>(null)

const params = ref<P | null>(null)

const {
  data,
  status,
  error,
  refresh,
  clear,
} = useAsyncData<T>(
  useId(),
  async () => await props.handler(params.value),
  props.options,
)

async function submit(raw: P) {
  validation.value = null
  const parsed = props.schema.safeParse(raw)
  if (parsed.success) params.value = parsed.data
  else validation.value = parsed.error
  await refresh()
}

function reset() {
  params.value = null
  validation.value = null
  clear()
}

type Slots = {
  submit: typeof submit
  reset: typeof reset
  data: typeof data.value
  status: typeof status.value
  error: typeof error.value
  validation: typeof validation.value
}

defineSlots<{ default(props: Slots): Slots }>()
</script>

<template>
  <slot
    :submit="submit"
    :reset="reset"
    :data="data"
    :status="status"
    :error="error"
    :validation="validation"
  />
</template>

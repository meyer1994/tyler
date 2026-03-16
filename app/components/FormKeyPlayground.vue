<script setup lang="ts">
import type { SelectItem } from '@nuxt/ui'
import * as z from 'zod'

const schema = z.object({
  method: z.enum(['GET', 'POST', 'PUT', 'DELETE']),
  endpoint: z.string().min(1, 'Endpoint is required'),
  apiKey: z.string().min(1, 'API Key is required'),
  body: z.string().optional(),
})

type Schema = z.output<typeof schema>
export type FormKeyPlaygroundData = Schema

type Props = {
  defaultValue?: Partial<Schema>
  loading?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  defaultValue: () => ({ method: 'GET', endpoint: '/api/protected/test' }),
  loading: false,
})

const state = reactive<Partial<Schema>>(props.defaultValue)

const emits = defineEmits<{ submit: [e: Schema] }>()

watch(() => props.defaultValue, v => v && Object.assign(state, v))

const methodOptions: SelectItem[] = [
  { label: 'GET', value: 'GET' },
  { label: 'POST', value: 'POST' },
  { label: 'PUT', value: 'PUT' },
  { label: 'DELETE', value: 'DELETE' },
]

const showBody = computed(() => state.method === 'POST' || state.method === 'PUT')
</script>

<template>
  <UForm
    :schema="schema"
    :state="state"
    class="flex flex-col gap-4"
    @submit.prevent="(e) => emits('submit', e.data)"
  >
    <div class="flex gap-4">
      <UFormField
        name="method"
        label="Method"
        class="w-32"
      >
        <USelect
          v-model="state.method"
          :items="methodOptions"
          value-key="value"
          class="w-full"
        />
      </UFormField>

      <UFormField
        name="endpoint"
        label="Endpoint"
        class="flex-1"
      >
        <UInput
          v-model="state.endpoint"
          placeholder="/api/protected/test"
          class="w-full font-mono"
        />
      </UFormField>
    </div>

    <UFormField
      name="apiKey"
      label="API Key"
      description="Paste your API key to authenticate the request"
    >
      <UInput
        v-model="state.apiKey"
        type="password"
        placeholder="your_api_key_here"
        class="w-full font-mono"
      />
    </UFormField>

    <UFormField
      v-if="showBody"
      name="body"
      label="Request Body"
      description="JSON body for the request"
    >
      <UTextarea
        v-model="state.body"
        placeholder="{ &quot;key&quot;: &quot;value&quot; }"
        :rows="4"
        class="w-full font-mono"
      />
    </UFormField>

    <div class="flex gap-2 justify-end items-center">
      <slot name="submit-button">
        <UButton
          type="submit"
          icon="i-lucide-send"
          label="Send Request"
          :loading="props.loading"
        />
      </slot>
    </div>
  </UForm>
</template>

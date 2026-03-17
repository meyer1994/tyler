<script setup lang="ts">
import type { SelectItem } from '@nuxt/ui'
import * as z from 'zod'

const schema = z.object({
  name: z.string().min(1, 'Name is required'),
  prefix: z.string().optional(),
  expiresIn: z.number().optional(),
})

type Schema = z.output<typeof schema>
export type FormApiKeyData = Schema

type Props = { defaultValue?: Partial<Schema>, loading?: boolean }

const props = withDefaults(defineProps<Props>(), {
  defaultValue: () => ({}),
  loading: () => false,
})

const state = reactive<Partial<Schema>>(props.defaultValue)
const emits = defineEmits<{ submit: [e: Schema] }>()

const expirationOptions: SelectItem[] = [
  { label: 'Never', value: undefined },
  { label: '7 days', value: 60 * 60 * 24 * 7 },
  { label: '30 days', value: 60 * 60 * 24 * 30 },
  { label: '90 days', value: 60 * 60 * 24 * 90 },
  { label: '1 year', value: 60 * 60 * 24 * 365 },
]

const selectedExpiration = ref<number | undefined>(undefined)
watch(selectedExpiration, v => state.expiresIn = v ?? undefined)
</script>

<template>
  <UForm
    :schema="schema"
    :state="state"
    class="flex flex-col gap-4"
    @submit.prevent="(e) => emits('submit', e.data)"
  >
    <UFormField
      name="name"
      label="Name"
      description="A friendly name to identify this key"
    >
      <UInput
        v-model="state.name"
        placeholder="e.g., Production API Key"
        class="w-full"
      />
    </UFormField>

    <UFormField
      name="prefix"
      label="Prefix"
      description="Optional prefix for the key (e.g., 'prod' will generate 'prod_xxx...')"
    >
      <UInput
        v-model="state.prefix"
        placeholder="e.g., prod"
        class="w-full"
      />
    </UFormField>

    <UFormField
      name="expiresIn"
      label="Expiration"
      description="When should this key expire?"
    >
      <USelect
        v-model="selectedExpiration"
        :items="expirationOptions"
        value-key="value"
        class="w-full"
      />
    </UFormField>

    <div class="flex gap-2 justify-end items-center mt-4">
      <slot name="cancel-button" />
      <slot name="submit-button">
        <UButton type="submit">
          Create Key
        </UButton>
      </slot>
    </div>
  </UForm>
</template>

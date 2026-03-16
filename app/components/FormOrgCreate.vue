<script setup lang="ts">
import * as z from 'zod'

const schema = z.object({
  name: z.string().min(1, 'Name is required'),
  slug: z.string().min(1, 'Slug is required').regex(/^[a-z0-9-]+$/, 'Slug must be lowercase alphanumeric'),
})

type Schema = z.output<typeof schema>
export type FormOrgCreateData = Schema

type Props = {
  defaultValue?: Partial<Schema>
  loading?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  defaultValue: () => ({}),
  loading: () => false,
})

const state = reactive<Partial<Schema>>(props.defaultValue)
watch(
  () => props.defaultValue,
  (v) => v && Object.assign(state, v),
)

const emits = defineEmits<{ submit: [e: Schema] }>()
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
    >
      <UInput
        v-model="state.name"
        placeholder="My Organization"
        class="w-full"
      />
    </UFormField>

    <UFormField
      name="slug"
      label="Slug"
    >
      <UInput
        v-model="state.slug"
        placeholder="my-org"
        class="w-full"
      />
    </UFormField>

    <UButton
      type="submit"
      :loading="props.loading"
      label="Create"
    />
  </UForm>
</template>

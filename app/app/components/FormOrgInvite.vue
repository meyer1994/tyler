<script setup lang="ts">
import * as z from 'zod'

const schema = z.object({
  email: z.string().email('Invalid email'),
  role: z.enum(['member', 'admin']),
})

type Schema = z.output<typeof schema>
export type FormOrgInviteData = Schema

type Props = {
  defaultValue?: Partial<Schema>
  loading?: boolean
}

const props = withDefaults(defineProps<Props>(), {
  defaultValue: () => ({ role: 'member' as const }),
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
    <div class="flex gap-4">
      <UFormField
        name="email"
        label="Email"
        class="flex-1"
      >
        <UInput
          v-model="state.email"
          type="email"
          placeholder="user@example.com"
          autocomplete="email"
          class="w-full"
        />
      </UFormField>

      <UFormField
        name="role"
        label="Role"
      >
        <USelect
          v-model="state.role"
          :items="[
            { label: 'Member', value: 'member' },
            { label: 'Admin', value: 'admin' },
          ]"
          class="w-32"
        />
      </UFormField>
    </div>

    <UButton
      type="submit"
      :loading="props.loading"
      label="Send Invitation"
    />
  </UForm>
</template>

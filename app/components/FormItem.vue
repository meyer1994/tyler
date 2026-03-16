<script setup lang="ts">
import * as z from 'zod'

const schema = z.object({
  name: z.string().min(1),
})

export type FormNameData = Schema

type Schema = z.output<typeof schema>
type Props = { defaultValue?: Partial<Schema>, loading?: boolean }

const props = withDefaults(defineProps<Props>(), {
  loading: () => false,
  defaultValue: () => ({}),
})

const state = reactive<Partial<Schema>>({ name: props.defaultValue?.name ?? '' })
watch(() => props.defaultValue, v => v && Object.assign(state, v))

const emits = defineEmits<{ submit: [e: Schema] }>()
</script>

<template>
  <UForm
    :schema="schema"
    :state="state"
    :disabled="loading"
    class="flex flex-col gap-4"
    @submit.prevent="(e) => emits('submit', e.data)"
  >
    <UFormField
      name="name"
      label="Nome"
      description="Digite o nome"
    >
      <UInput
        v-model="state.name"
        placeholder="Ex: João da Silva"
        class="w-full"
      />
    </UFormField>

    <div class="flex gap-2 justify-between items-center mt-4">
      <slot name="submit-button">
        <UButton type="submit">
          Salvar
        </UButton>
      </slot>
    </div>
  </UForm>
</template>

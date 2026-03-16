<script setup lang="ts">
import * as z from 'zod'

const schema = z.object({
  file: z.instanceof(File)
    .refine(f => f.size > 0, 'File is required')
    .refine(f => f.size <= 10 * 1024 * 1024, 'File size must be less than 10MB')
    .refine(
      f => f.type === 'text/plain' || f.type === 'text/markdown',
      'File must be a text file (plain or markdown)',
    )
    .refine(f => f.name.trim().length > 0, 'File name is required'),
})

type Schema = z.output<typeof schema>
export type FormFileData = Schema

type Props = { defaultValue?: Partial<Schema>, loading?: boolean }
const props = withDefaults(defineProps<Props>(), {
  defaultValue: () => ({}),
  loading: () => false,
})

const state = reactive<Partial<Schema>>({ file: props.defaultValue?.file })
watch(() => props.defaultValue, v => v && Object.assign(state, v))

const emits = defineEmits<{ submit: [e: File] }>()
</script>

<template>
  <UForm
    :schema="schema"
    :state="state"
    :disabled="loading"
    class="flex flex-col gap-4"
    @submit.prevent="(e) => emits('submit', e.data.file)"
  >
    <UFormField
      name="file"
      label="Arquivo"
      description="Selecione uma imagem (máximo 10MB)"
    >
      <UFileUpload
        v-model="state.file"
        accept=".md,.txt,text/plain,text/markdown"
      />
    </UFormField>

    <div class="flex gap-2 justify-between items-center mt-4">
      <slot name="submit-button">
        <UButton type="submit">
          Enviar
        </UButton>
      </slot>
    </div>
  </UForm>
</template>

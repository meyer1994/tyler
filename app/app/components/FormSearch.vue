<script setup lang="ts">
import * as z from 'zod'

const schema = z.object({
  query: z.string().min(1, 'Digite algo para pesquisar'),
  prefix: z.string().optional(),
})

type Schema = z.output<typeof schema>
export type FormSearchData = Schema

type Props = { defaultValue?: Partial<Schema>, loading?: boolean }
const props = withDefaults(defineProps<Props>(), {
  defaultValue: () => ({}),
  loading: () => false,
})

const state = reactive<Partial<Schema>>({
  query: props.defaultValue?.query ?? '',
  prefix: props.defaultValue?.prefix ?? '',
})

const emits = defineEmits<{ submit: [e: Schema] }>()

const form = useTemplateRef('form')

// Sync props changes to state
watch(() => props.defaultValue, v => v && Object.assign(state, v))
</script>

<template>
  <UForm
    ref="form"
    :schema="schema"
    :state="state"
    class="flex flex-col gap-4"
    @submit.prevent="(e) => emits('submit', e.data)"
  >
    <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
      <UFormField
        name="query"
        label="Query"
        description="Semantic search query"
      >
        <UInput
          v-model="state.query"
          placeholder="e.g. How to use LangChain?"
          class="w-full"
          icon="i-lucide-search"
          @keydown.enter="form?.submit()"
        />
      </UFormField>

      <UFormField
        name="prefix"
        label="Prefix"
        description="Filter by file path prefix"
      >
        <UInput
          v-model="state.prefix"
          placeholder="e.g. docs/"
          class="w-full"
          icon="i-lucide-folder"
          @keydown.enter="form?.submit()"
        />
      </UFormField>
    </div>

    <div class="flex gap-2 justify-between items-center mt-2">
      <slot name="submit-button">
        <UButton
          type="submit"
          icon="i-lucide-sparkles"
        >
          Pesquisar
        </UButton>
      </slot>
    </div>
  </UForm>
</template>

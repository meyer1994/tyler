<script setup lang="ts">
import * as z from 'zod'

const schema = z.object({
  message: z.string().min(1, 'Message is required'),
})

type Schema = z.output<typeof schema>
export type FormQueueMessageData = Schema

const state = reactive<Partial<Schema>>({ message: '' })

const emits = defineEmits<{ submit: [e: Schema] }>()

function onSubmit() {
  emits('submit', state as Schema)
  state.message = ''
}
</script>

<template>
  <UForm
    :schema="schema"
    :state="state"
    class="flex flex-col gap-4"
    @submit.prevent="onSubmit"
  >
    <UFormField
      name="message"
      label="Message"
      description="Enter the message to send to the queue"
    >
      <UTextarea
        v-model="state.message"
        placeholder="Enter your message..."
        :rows="4"
        class="w-full"
      />
    </UFormField>

    <div class="flex gap-2">
      <slot name="submit-button">
        <UButton
          type="submit"
          icon="i-lucide-send"
        >
          Send to Queue
        </UButton>
      </slot>
    </div>
  </UForm>
</template>

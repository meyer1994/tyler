<script setup lang="ts">
import type { FormQueueMessageData } from '~/components/FormQueueMessage.vue'

const { $trpc } = useNuxtApp()

// Fetch tasks from database
const { data: tasks, refresh } = await useAsyncData(
  'tasks',
  () => $trpc.queue.list.query(),
)

const loading = ref(false)

async function onSubmit(data: FormQueueMessageData) {
  loading.value = true
  try {
    await $trpc.queue.send.mutate({ message: data.message })
    await refresh()
  }
  finally {
    loading.value = false
  }
}

const statusConfig = {
  PENDING: { icon: 'i-lucide-clock', color: 'text-muted', label: 'Pending' },
  SENDING: { icon: 'i-lucide-loader', color: 'text-info', label: 'Sending' },
  RUNNING: { icon: 'i-lucide-loader', color: 'text-info', label: 'Running' },
  ERRORED: { icon: 'i-lucide-x-circle', color: 'text-error', label: 'Errored' },
  FINISHED: { icon: 'i-lucide-check-circle', color: 'text-success', label: 'Finished' },
} as const

async function onDelete(id: string) {
  await $trpc.queue.delete.mutate({ id })
  await refresh()
}
</script>

<template>
  <div class="flex flex-col gap-4">
    <UCard>
      <template #header>
        <div>
          <h1 class="text-xl font-semibold">
            Queue
          </h1>
          <p class="text-sm text-muted mt-1">
            Send messages to Cloudflare Queue
          </p>
        </div>
      </template>

      <FormQueueMessage @submit="onSubmit">
        <template #submit-button>
          <UButton
            type="submit"
            :loading="loading"
            icon="i-lucide-send"
          >
            Send to Queue
          </UButton>
        </template>
      </FormQueueMessage>
    </UCard>

    <!-- Tasks -->
    <UCard>
      <template #header>
        <div class="flex items-center justify-between">
          <div>
            <h2 class="text-lg font-semibold">
              Tasks
            </h2>
            <p class="text-sm text-muted mt-1">
              Queue tasks and their status
            </p>
          </div>
          <UButton
            variant="ghost"
            color="neutral"
            icon="i-lucide-refresh-cw"
            size="sm"
            @click="refresh()"
          >
            Refresh
          </UButton>
        </div>
      </template>

      <div
        v-if="tasks && tasks.length > 0"
        class="flex flex-col gap-3"
      >
        <div
          v-for="task in tasks"
          :key="task.id"
          class="flex items-start gap-3 p-3 rounded-lg bg-muted/50"
        >
          <UIcon
            :name="statusConfig[task.status].icon"
            :class="[statusConfig[task.status].color, task.status === 'RUNNING' && 'animate-spin']"
            class="size-5 mt-0.5 shrink-0"
          />
          <div class="flex-1 min-w-0">
            <div class="flex items-center gap-2 mb-1">
              <UBadge
                :color="task.status === 'FINISHED' ? 'success' : task.status === 'ERRORED' ? 'error' : task.status === 'RUNNING' ? 'info' : 'neutral'"
                variant="subtle"
                size="xs"
              >
                {{ statusConfig[task.status].label }}
              </UBadge>
              <span class="text-xs text-muted font-mono">{{ task.id.slice(0, 8) }}</span>
            </div>
            <p class="text-sm wrap-break-word">
              {{ task.message }}
            </p>
            <p
              v-if="task.error"
              class="text-xs text-error mt-1"
            >
              Error: {{ task.error }}
            </p>
            <p class="text-xs text-muted mt-1">
              {{ new Date(task.createdAt).toLocaleString() }}
            </p>
          </div>
          <UButton
            variant="ghost"
            color="neutral"
            icon="i-lucide-trash-2"
            size="xs"
            @click="onDelete(task.id)"
          />
        </div>
      </div>

      <div
        v-else
        class="text-center py-8 text-muted"
      >
        <UIcon
          name="i-lucide-inbox"
          class="size-12 mx-auto mb-2 opacity-50"
        />
        <p>No tasks yet</p>
        <p class="text-sm">
          Send a message to create your first task
        </p>
      </div>
    </UCard>
  </div>
</template>

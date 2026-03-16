<script setup lang="ts">
import type { FormKeyPlaygroundData } from '~/components/FormKeyPlayground.vue'
import type { RequestHistoryItem } from '~/components/TableKeyRequests.vue'

const loading = ref(false)
const history = ref<RequestHistoryItem[]>([])

// Response viewer state
const showResponse = ref(false)
const selectedRequest = ref<RequestHistoryItem | null>(null)

async function onSubmit(formData: FormKeyPlaygroundData) {
  loading.value = true
  const startTime = performance.now()

  const requestItem: RequestHistoryItem = {
    id: crypto.randomUUID(),
    timestamp: new Date(),
    method: formData.method,
    endpoint: formData.endpoint,
    status: 0,
    duration: 0,
    requestBody: formData.body,
    responseBody: '',
  }

  try {
    const response = await $fetch.raw(formData.endpoint, {
      method: formData.method,
      headers: { 'x-api-key': formData.apiKey, 'Content-Type': 'application/json' },
      body: formData.body && (formData.method === 'POST' || formData.method === 'PUT')
        ? JSON.parse(formData.body)
        : undefined,
      ignoreResponseError: true,
    })

    const endTime = performance.now()
    requestItem.duration = Math.round(endTime - startTime)
    requestItem.status = response.status
    requestItem.responseBody = JSON.stringify(response._data, null, 2)
  }
  catch (error) {
    const endTime = performance.now()
    requestItem.duration = Math.round(endTime - startTime)
    requestItem.status = 0
    requestItem.error = error instanceof Error ? error.message : 'Unknown error'
    requestItem.responseBody = JSON.stringify({ error: requestItem.error }, null, 2)
  }
  finally {
    loading.value = false
    history.value.unshift(requestItem)
  }
}

function onViewResponse(item: RequestHistoryItem) {
  selectedRequest.value = item
  showResponse.value = true
}

function onClearHistory() {
  history.value = []
}
</script>

<template>
  <div class="flex flex-col gap-4">
    <UCard>
      <template #header>
        <div>
          <h1 class="text-xl font-semibold">
            Playground
          </h1>
          <p class="text-sm text-muted mt-1">
            Test your API keys with live requests
          </p>
        </div>
      </template>

      <FormKeyPlayground
        :loading="loading"
        @submit="onSubmit"
      />

      <TableKeyRequests
        :items="history"
        :loading="loading"
        @view-response="onViewResponse"
        @clear-history="onClearHistory"
      />
    </UCard>

    <!-- Response Viewer Slideover -->
    <USlideover
      v-model:open="showResponse"
      title="Response Details"
      description="View the response details for the request"
      :ui="{ footer: 'justify-end' }"
    >
      <template #body>
        <div class="flex flex-col gap-4">
          <!-- Request Info -->
          <div class="flex flex-col gap-2">
            <label class="text-sm font-medium">Request</label>
            <div class="flex items-center gap-2">
              <UBadge
                variant="subtle"
                class="font-mono"
              >
                {{ selectedRequest?.method }}
              </UBadge>
              <code class="font-mono text-sm">{{ selectedRequest?.endpoint ?? 'No endpoint' }}</code>
            </div>
          </div>

          <!-- Status -->
          <div class="flex flex-col gap-2">
            <label class="text-sm font-medium">Status</label>
            <div class="flex items-center gap-2">
              <UBadge variant="subtle">
                {{ selectedRequest?.status || 'Error' }}
              </UBadge>
              <span class="text-sm text-muted">
                {{ selectedRequest?.duration }}ms
              </span>
            </div>
          </div>

          <!-- Request Body (if any) -->
          <div
            v-if="selectedRequest?.requestBody"
            class="flex flex-col gap-2"
          >
            <label class="text-sm font-medium">Request Body</label>
            <pre class="bg-muted p-3 rounded-lg text-sm font-mono overflow-auto max-h-32">{{ selectedRequest?.requestBody ?? 'No request body' }}</pre>
          </div>

          <!-- Response Body -->
          <div class="flex flex-col gap-2">
            <label class="text-sm font-medium">Response Body</label>
            <pre class="bg-muted p-3 rounded-lg text-sm font-mono overflow-auto max-h-64">{{ selectedRequest?.responseBody ?? 'No response body' }}</pre>
          </div>

          <!-- Error (if any) -->
          <UAlert
            v-if="selectedRequest?.error"
            icon="i-lucide-alert-circle"
            color="error"
            title="Error"
            :description="selectedRequest.error"
          />
        </div>
      </template>

      <template #footer>
        <UButton
          label="Close"
          color="neutral"
          variant="outline"
          @click="showResponse = false"
        />
      </template>
    </USlideover>
  </div>
</template>

<script setup lang="ts">
import type { TableColumn } from '@nuxt/ui'

export type RequestHistoryItem = {
  id: string
  timestamp: Date
  method: 'GET' | 'POST' | 'PUT' | 'DELETE'
  endpoint: string
  status: number
  duration: number
  requestBody?: string
  responseBody: string
  error?: string
}

type Props = {
  items: RequestHistoryItem[]
  loading?: boolean
}

const props = defineProps<Props>()
const emit = defineEmits<{
  'view-response': [item: RequestHistoryItem]
  'clear-history': []
}>()

type Keys = keyof RequestHistoryItem | 'actions'

const columns: TableColumn<RequestHistoryItem>[] = [
  {
    id: 'timestamp' as const,
    accessorKey: 'timestamp',
    header: 'Time',
  },
  {
    id: 'method' as const,
    accessorKey: 'method',
    header: 'Method',
  },
  {
    id: 'endpoint' as const,
    accessorKey: 'endpoint',
    header: 'Endpoint',
  },
  {
    id: 'status' as const,
    accessorKey: 'status',
    header: 'Status',
  },
  {
    id: 'duration' as const,
    accessorKey: 'duration',
    header: 'Duration',
  },
  {
    id: 'actions' as const,
    header: 'Actions',
  },
]

const MAP_ID_TO_LABEL: Record<string, string> = columns
  .reduce((a, c) => ({ ...a, [c.id as string]: c.header }), {})

const visible: Ref<Record<Keys, boolean>> = ref({
  id: false,
  timestamp: false,
  method: true,
  endpoint: false,
  status: true,
  duration: true,
  requestBody: false,
  responseBody: false,
  error: false,
  actions: true,
})

const table = useTemplateRef('table')

function getMethodColor(method: string) {
  const colors: Record<string, 'info' | 'success' | 'warning' | 'error'> = {
    GET: 'info',
    POST: 'success',
    PUT: 'warning',
    DELETE: 'error',
  }
  return colors[method] ?? 'neutral'
}

function getStatusColor(status: number) {
  if (status >= 200 && status < 300) return 'success'
  if (status >= 400 && status < 500) return 'warning'
  if (status >= 500) return 'error'
  return 'neutral'
}
</script>

<template>
  <div class="flex flex-col flex-1 w-full">
    <div class="flex items-center gap-2 py-3.5 border-b border-accented">
      <span class="text-sm font-medium">Request History</span>

      <UButton
        v-if="props.items.length > 0"
        icon="i-lucide-trash-2"
        color="neutral"
        variant="ghost"
        size="sm"
        title="Clear history"
        aria-label="Clear history"
        class="ml-auto"
        @click="emit('clear-history')"
      />

      <UDropdownMenu
        :items="
          table
            ?.tableApi
            ?.getAllColumns()
            .filter((column) => column.getCanHide())
            .map((column) => ({
              label: (MAP_ID_TO_LABEL[column.id] ?? column.id),
              type: 'checkbox' as const,
              checked: column.getIsVisible(),
              onUpdateChecked: (checked: boolean) =>
                table
                  ?.tableApi
                  ?.getColumn(column.id)
                  ?.toggleVisibility(!!checked),
              onSelect: (e: Event) => e.preventDefault(),
            }))
        "
        :content="{ align: 'end' }"
      >
        <UButton
          label="Columns"
          color="neutral"
          variant="outline"
          size="sm"
          trailing-icon="i-lucide-chevron-down"
          aria-label="Select columns"
        />
      </UDropdownMenu>
    </div>

    <UTable
      ref="table"
      v-model:column-visibility="visible"
      :loading="props.loading"
      loading-animation="carousel"
      :columns="columns"
      :data="props.items"
      :sticky="true"
      :empty-state="{
        icon: 'i-lucide-send',
        label: 'No requests yet',
        description: 'Send a request to see it here',
      }"
      :ui="{
        tr: 'hover:bg-muted',
      }"
      class="flex-1"
    >
      <!-- Timestamp Column -->
      <template #timestamp-cell="{ row }">
        <NuxtTime
          :datetime="row.original.timestamp"
          title
        />
      </template>

      <!-- Method Column -->
      <template #method-cell="{ row }">
        <UBadge
          :color="getMethodColor(row.original.method)"
          variant="subtle"
          class="font-mono"
        >
          {{ row.original.method }}
        </UBadge>
      </template>

      <!-- Endpoint Column -->
      <template #endpoint-cell="{ row }">
        <code class="font-mono text-sm">
          {{ row.original.endpoint }}
        </code>
      </template>

      <!-- Status Column -->
      <template #status-cell="{ row }">
        <UBadge
          :color="getStatusColor(row.original.status)"
          variant="subtle"
        >
          {{ row.original.status }}
        </UBadge>
      </template>

      <!-- Duration Column -->
      <template #duration-cell="{ row }">
        <span class="text-sm text-muted">
          {{ row.original.duration }}ms
        </span>
      </template>

      <!-- Actions Column -->
      <template #actions-cell="{ row }">
        <div class="flex items-center gap-1">
          <UButton
            icon="i-lucide-eye"
            color="neutral"
            variant="ghost"
            size="sm"
            title="View response"
            aria-label="View response"
            @click="emit('view-response', row.original)"
          />
        </div>
      </template>
    </UTable>
  </div>
</template>

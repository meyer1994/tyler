<script setup lang="ts">
import type { TableColumn } from '@nuxt/ui'
import type { ApiKey } from 'better-auth/plugins'

type Props = {
  items: ApiKey[]
  loading?: boolean
}

const props = defineProps<Props>()
const emit = defineEmits<{
  'delete-key': [key: ApiKey]
  'select-key': [key: ApiKey]
  'refresh-table': []
}>()

type Keys = keyof ApiKey | 'actions'

const columns: TableColumn<ApiKey>[] = [
  {
    id: 'name' as const,
    accessorKey: 'name',
    header: 'Name',
  },
  {
    id: 'start' as const,
    accessorKey: 'start',
    header: 'Key',
  },
  {
    id: 'enabled' as const,
    accessorKey: 'enabled',
    header: 'Status',
  },
  {
    id: 'expiresAt' as const,
    accessorKey: 'expiresAt',
    header: 'Expires',
  },
  {
    id: 'createdAt' as const,
    accessorKey: 'createdAt',
    header: 'Created',
  },
  {
    id: 'actions' as const,
    header: 'Actions',
  },
]

const MAP_ID_TO_LABEL: Record<string, string> = columns
  .reduce((a, c) => ({ ...a, [c.id as string]: c.header }), {})

const visible = ref<Record<Keys, boolean>>({
  id: false,
  name: true,
  start: true,
  prefix: false,
  key: false,
  userId: false,
  refillInterval: false,
  refillAmount: false,
  lastRefillAt: false,
  enabled: true,
  rateLimitEnabled: false,
  rateLimitTimeWindow: false,
  rateLimitMax: false,
  requestCount: false,
  remaining: false,
  lastRequest: false,
  expiresAt: true,
  createdAt: true,
  updatedAt: false,
  metadata: false,
  permissions: false,
  actions: true,
})

const table = useTemplateRef('table')
</script>

<template>
  <div class="flex flex-col flex-1 w-full">
    <div class="flex items-center gap-2 py-3.5 border-b border-accented">
      <slot name="top" />

      <UButton
        icon="i-lucide-refresh-cw"
        color="neutral"
        variant="ghost"
        size="sm"
        title="Refresh"
        aria-label="Refresh"
        class="ml-auto"
        @click="emit('refresh-table')"
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
        icon: 'i-lucide-key',
        label: 'No API keys found',
      }"
      :ui="{
        tr: 'hover:bg-muted',
      }"
      class="flex-1"
    >
      <!-- Name Column -->
      <template #name-cell="{ row }">
        <span class="text-sm font-medium">
          {{ row.original.name || 'Unnamed' }}
        </span>
      </template>

      <!-- Key Preview Column -->
      <template #start-cell="{ row }">
        <code class="font-mono text-sm bg-muted px-2 py-1 rounded">
          {{ row.original.prefix ? `${row.original.prefix}_` : '' }}{{ row.original.start || '...' }}...
        </code>
      </template>

      <!-- Status Column -->
      <template #enabled-cell="{ row }">
        <UBadge
          :color="row.original.enabled ? 'success' : 'error'"
          variant="subtle"
        >
          {{ row.original.enabled ? 'Active' : 'Disabled' }}
        </UBadge>
      </template>

      <!-- Expires Column -->
      <template #expiresAt-cell="{ row }">
        <NuxtTime
          v-if="row.original.expiresAt"
          :datetime="row.original.expiresAt"
          title
        />
        <span
          v-else
          class="text-muted text-sm"
        >Never</span>
      </template>

      <!-- Created Column -->
      <template #createdAt-cell="{ row }">
        <NuxtTime
          :datetime="row.original.createdAt"
          title
        />
      </template>

      <!-- Actions Column -->
      <template #actions-cell="{ row }">
        <div class="flex items-center gap-1">
          <UButton
            icon="i-lucide-trash-2"
            color="error"
            variant="ghost"
            size="sm"
            title="Delete key"
            aria-label="Delete key"
            @click="emit('delete-key', row.original)"
          />
        </div>
      </template>
    </UTable>
  </div>
</template>

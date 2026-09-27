<script setup lang="ts">
import type { AppRouterOutputs } from '@@/server/trpc'
import type { TableColumn } from '@nuxt/ui'

type Row = AppRouterOutputs['boards']['list'][number]
type Keys = 'status' | 'id' | 'name' | 'ip' | 'count' | 'lastSeenAt' | 'createdAt' | 'updatedAt'

const ONLINE_WINDOW_MS = 2 * 60 * 1000

const columns: TableColumn<Row>[] = [
  { id: 'status', header: 'Status' },
  { id: 'id', accessorKey: 'id', header: 'MAC' },
  { id: 'name', accessorKey: 'name', header: 'Name' },
  { id: 'ip', accessorKey: 'ip', header: 'IP' },
  { id: 'count', accessorKey: 'eventsCount', header: 'Events' },
  { id: 'lastSeenAt', accessorKey: 'lastSeenAt', header: 'Last Seen' },
  { id: 'createdAt', accessorKey: 'createdAt', header: 'Created At' },
  { id: 'updatedAt', accessorKey: 'updatedAt', header: 'Updated At' },
]

const MAP_ID_TO_LABEL: Record<Keys, string> = columns.reduce(
  (labels, column) => ({ ...labels, [column.id as Keys]: column.header }),
  {} as Record<Keys, string>,
)

const visible: Ref<Record<Keys, boolean>> = ref({
  status: true,
  id: true,
  name: false,
  ip: true,
  count: true,
  lastSeenAt: true,
  createdAt: true,
  updatedAt: false,
})

const table = useTemplateRef('table')
const { $trpc } = useNuxtApp()
const now = useNow({ interval: 30_000 })

const isOnline = (lastSeenAt: Row['lastSeenAt']) =>
  !!lastSeenAt && now.value.getTime() - new Date(lastSeenAt).getTime() < ONLINE_WINDOW_MS
</script>

<template>
  <div class="h-full w-full p-4 flex flex-col gap-6">
    <AsyncFetch
      v-slot="{ data, status, refresh }"
      fetch-key="boards"
      :handler="async () => await $trpc.boards.list.query()"
    >
      <div class="flex flex-col flex-1 w-full">
        <div class="flex items-center gap-2 py-3.5 border-b border-accented">
          <UButton
            icon="i-lucide-refresh-cw"
            color="neutral"
            variant="ghost"
            size="sm"
            class="ml-auto"
            @click="refresh()"
          />

          <UDropdownMenu
            :items="
              table?.tableApi
                ?.getAllColumns()
                .filter(column => column.getCanHide())
                .map(column => ({
                  label: MAP_ID_TO_LABEL[column.id as Keys] ?? column.id,
                  type: 'checkbox' as const,
                  checked: column.getIsVisible(),
                  onUpdateChecked: (checked: boolean) =>
                    table?.tableApi?.getColumn(column.id)?.toggleVisibility(!!checked),
                  onSelect: (event: Event) => event.preventDefault(),
                }))
            "
            :content="{ align: 'end' }"
          >
            <UButton
              label="Columns"
              color="neutral"
              variant="outline"
              trailing-icon="i-lucide-chevron-down"
            />
          </UDropdownMenu>
        </div>

        <UTable
          ref="table"
          v-model:column-visibility="visible"
          :loading="status === 'pending'"
          loading-animation="carousel"
          :columns="columns"
          :data="data ?? []"
          :sticky="true"
          :empty-state="{
            icon: 'i-lucide-cpu',
            label: 'No boards found',
          }"
          :ui="{
            tr: 'hover:bg-muted cursor-pointer',
          }"
          class="flex-1"
        >
          <template #status-cell="{ row }">
            <UBadge
              :color="isOnline(row.original.lastSeenAt) ? 'success' : 'neutral'"
              :label="isOnline(row.original.lastSeenAt) ? 'Online' : 'Offline'"
              variant="subtle"
            />
          </template>

          <template #id-cell="{ row }">
            <span class="font-mono text-sm text-primary font-medium">
              {{ row.original.id }}
            </span>
          </template>

          <template #name-cell="{ row }">
            {{ row.original.name ?? '-' }}
          </template>

          <template #ip-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.ip ?? '-' }}
            </span>
          </template>

          <template #count-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.eventsCount }}
            </span>
          </template>

          <template #lastSeenAt-cell="{ row }">
            <NuxtTime
              v-if="row.original.lastSeenAt"
              :datetime="row.original.lastSeenAt"
              title
              relative
            />
            <span v-else>-</span>
          </template>

          <template #createdAt-cell="{ row }">
            <NuxtTime
              :datetime="row.original.createdAt"
              title
            />
          </template>

          <template #updatedAt-cell="{ row }">
            <NuxtTime
              :datetime="row.original.updatedAt"
              title
            />
          </template>
        </UTable>
      </div>
    </AsyncFetch>
  </div>
</template>

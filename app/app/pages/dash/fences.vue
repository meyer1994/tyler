<script setup lang="ts">
import type { AppRouterOutputs } from '@@/server/trpc'
import type { TableColumn } from '@nuxt/ui'

type Item = AppRouterOutputs['fences']['list']
type Row = Item['features'][number]
type Keys = keyof Row['properties'] | 'count' | 'actions'

const columns: TableColumn<Row>[] = [
  { id: 'id', accessorKey: 'properties.id', header: 'ID' },
  { id: 'area', accessorKey: 'properties.area', header: 'Area' },
  { id: 'count', accessorKey: 'properties.eventsCount', header: 'Count' },
  { id: 'createdAt', accessorKey: 'properties.createdAt', header: 'Created At' },
  { id: 'updatedAt', accessorKey: 'properties.updatedAt', header: 'Updated At' },
  { id: 'actions', header: 'Actions' },
]

const MAP_ID_TO_LABEL: Record<Keys, string> = columns.reduce(
  (labels, column) => ({ ...labels, [column.id as Keys]: column.header }),
  {} as Record<Keys, string>,
)

const visible: Ref<Record<Keys, boolean>> = ref({
  id: true,
  area: true,
  count: true,
  createdAt: true,
  updatedAt: false,
  actions: true,
})

const table = useTemplateRef('table')

const deleteFence = async (id: string, refresh: () => Promise<unknown>) => {
  await $trpc.fences.delete.mutate({ id })
  await refresh()
}
</script>

<template>
  <div class="h-full w-full p-4 flex flex-col gap-6">
    <AsyncFetch
      v-slot="{ data, status, refresh }"
      fetch-key="fences"
      :handler="async () => await $trpc.fences.list.query()"
    >
      <div class="flex flex-col flex-1 w-full">
        <div class="flex items-center gap-2 py-3.5 border-b border-accented">
          <UButton
            icon="i-lucide-refresh-cw"
            color="neutral"
            variant="ghost"
            size="sm"
            class="ml-auto"
            @click="refresh"
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
          :data="data?.features ?? []"
          :sticky="true"
          :empty-state="{
            icon: 'i-lucide-box',
            label: 'No fences found',
          }"
          :ui="{
            tr: 'hover:bg-muted cursor-pointer',
          }"
          class="flex-1"
        >
          <template #id-cell="{ row }">
            <span class="font-bold font-mono text-sm">
              #{{ row.original.properties.id.substring(0, 8) }}
            </span>
          </template>

          <template #area-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.properties.area.toFixed(2) }} m²
            </span>
          </template>

          <template #count-cell="{ row }">
            <span class="font-mono text-sm">
              {{ row.original.properties.eventsCount }}
            </span>
          </template>

          <template #createdAt-cell="{ row }">
            <NuxtTime
              :datetime="row.original.properties.createdAt"
              title
            />
          </template>

          <template #updatedAt-cell="{ row }">
            <NuxtTime
              :datetime="row.original.properties.updatedAt"
              title
            />
          </template>

          <template #actions-cell="{ row }">
            <UButton
              icon="i-lucide-trash-2"
              color="error"
              variant="ghost"
              size="sm"
              @click="deleteFence(row.original.properties.id, refresh)"
            />
          </template>
        </UTable>
      </div>
    </AsyncFetch>
  </div>
</template>

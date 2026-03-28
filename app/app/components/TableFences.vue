<script setup lang="ts">
import type { AppRouterOutputs } from '@@/server/trpc'
import type { TableColumn } from '@nuxt/ui'

type Item = AppRouterOutputs['fences']['list']
type Props = { item?: Item, filter?: string, loading?: boolean }
const props = withDefaults(defineProps<Props>(), {
  item: () => ({ type: 'FeatureCollection', features: [] }),
})

const emit = defineEmits<{
  'select-item': [item: Item['features'][number]]
  'delete-item': [item: Item['features'][number]]
  'refresh-table': []
}>()

type Keys = keyof Item['features'][number]['properties'] | 'actions'

const columns: TableColumn<Item['features'][number]>[] = [
  {
    id: 'id' as const,
    accessorKey: 'properties.id',
    header: 'ID',
  },
  {
    id: 'area' as const,
    accessorKey: 'properties.area',
    header: 'Area',
  },
  {
    id: 'count' as const,
    accessorKey: 'properties.eventsCount',
    header: 'Count',
  },
  {
    id: 'createdAt' as const,
    accessorKey: 'properties.createdAt',
    header: 'Created At',
  },
  {
    id: 'updatedAt' as const,
    accessorKey: 'properties.updatedAt',
    header: 'Updated At',
  },
  {
    id: 'actions' as const,
    header: 'Actions',
  },
]

const MAP_ID_TO_LABEL: Record<Keys, string> = columns.reduce(
  (a, c) => ({ ...a, [c.id as Keys]: c.header }),
  {} as Record<Keys, string>,
)

const visible: Ref<Record<Keys, boolean>> = ref({
  id: true,
  area: true,
  eventsCount: true,
  createdAt: true,
  updatedAt: false,
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
        class="ml-auto"
        @click="emit('refresh-table')"
      />

      <UDropdownMenu
        :items="
          table?.tableApi
            ?.getAllColumns()
            .filter((column) => column.getCanHide())
            .map((column) => ({
              label: MAP_ID_TO_LABEL[column.id as Keys] ?? column.id,
              type: 'checkbox' as const,
              checked: column.getIsVisible(),
              onUpdateChecked: (checked: boolean) =>
                table?.tableApi
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
        />
      </UDropdownMenu>
    </div>

    <UTable
      ref="table"
      v-model:column-visibility="visible"
      :loading="props.loading"
      loading-animation="carousel"
      :filter="props.filter"
      :global-filter="filter"
      :columns="columns"
      :data="props.item.features"
      :sticky="true"
      :empty-state="{
        icon: 'i-lucide-box',
        label: 'No fences found',
      }"
      :ui="{
        tr: 'hover:bg-muted cursor-pointer',
      }"
      class="flex-1"
      @select="(e, row) => emit('select-item', row.original)"
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
        <div class="flex items-center gap-1">
          <UButton
            icon="i-lucide-trash-2"
            color="error"
            variant="ghost"
            size="sm"
            @click="() => emit('delete-item', row.original)"
          />
        </div>
      </template>
    </UTable>
  </div>
</template>

<script setup lang="ts">
import type { AppRouterOutputs } from '@@/server/trpc'
import type { TableColumn } from '@nuxt/ui'

type Item = AppRouterOutputs['events']['list'][number]

type Props = {
  items: Item[]
  filter?: string
  loading?: boolean
}

const props = defineProps<Props>()

const emit = defineEmits<{
  'refresh-table': []
}>()

type Keys = keyof Item

const columns: TableColumn<Item>[] = [
  {
    id: 'boardId' as const,
    accessorKey: 'boardId',
    header: 'Board ID',
  },
  {
    id: 'lat' as const,
    accessorKey: 'lat',
    header: 'Latitude',
  },
  {
    id: 'lon' as const,
    accessorKey: 'lon',
    header: 'Longitude',
  },
  {
    id: 'alt' as const,
    accessorKey: 'alt',
    header: 'Altitude',
  },
  {
    id: 'speed' as const,
    accessorKey: 'speed',
    header: 'Speed',
  },
  {
    id: 'heading' as const,
    accessorKey: 'heading',
    header: 'Heading',
  },
  {
    id: 'hdop' as const,
    accessorKey: 'hdop',
    header: 'HDOP',
  },
  {
    id: 'sat' as const,
    accessorKey: 'sat',
    header: 'Satellites',
  },
  {
    id: 'ts' as const,
    accessorKey: 'ts',
    header: 'Timestamp',
  },
  {
    id: 'createdAt' as const,
    accessorKey: 'createdAt',
    header: 'Created At',
  },
]

const MAP_ID_TO_LABEL: Record<Keys, string> = columns.reduce(
  (a, c) => ({ ...a, [c.id as Keys]: c.header }),
  {} as Record<Keys, string>,
)

const visible: Ref<Record<Keys, boolean>> = ref({
  boardId: true,
  lat: true,
  lon: true,
  alt: false,
  speed: true,
  heading: false,
  hdop: true,
  sat: true,
  ts: false,
  createdAt: true,
  updatedAt: false,
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
      :data="props.items"
      :sticky="true"
      :empty-state="{
        icon: 'i-lucide-map-pin-off',
        label: 'No events found',
      }"
      :ui="{
        tr: 'hover:bg-muted cursor-pointer',
      }"
      class="flex-1"
    >
      <template #boardId-cell="{ row }">
        <span class="font-mono text-sm text-primary font-medium">{{ row.original.boardId }}</span>
      </template>

      <template #lat-cell="{ row }">
        <span class="font-mono text-sm">{{ row.original.lat.toFixed(6) }}</span>
      </template>

      <template #lon-cell="{ row }">
        <span class="font-mono text-sm">{{ row.original.lon.toFixed(6) }}</span>
      </template>

      <template #alt-cell="{ row }">
        <span class="font-mono text-sm">{{ row.original.alt ? `${row.original.alt.toFixed(1)}m` : '—' }}</span>
      </template>

      <template #speed-cell="{ row }">
        <span class="font-mono text-sm">{{ row.original.speed ? `${row.original.speed.toFixed(1)}km/h` : '—' }}</span>
      </template>

      <template #heading-cell="{ row }">
        <span class="font-mono text-sm">{{ row.original.heading ? `${row.original.heading.toFixed(0)}°` : '—' }}</span>
      </template>

      <template #hdop-cell="{ row }">
        <span class="font-mono text-sm">{{ row.original.hdop ? row.original.hdop.toFixed(2) : '—' }}</span>
      </template>

      <template #sat-cell="{ row }">
        <span class="font-mono text-sm font-medium">{{ row.original.sat ?? '—' }}</span>
      </template>

      <template #ts-cell="{ row }">
        <span class="font-mono text-sm">{{ row.original.ts ?? '—' }}</span>
      </template>

      <template #createdAt-cell="{ row }">
        <NuxtTime
          :datetime="row.original.createdAt"
          title
        />
      </template>
    </UTable>
  </div>
</template>

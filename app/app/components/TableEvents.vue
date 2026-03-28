<script setup lang="ts">
import type { AppRouterOutputs } from '@@/server/trpc'
import type { TableColumn } from '@nuxt/ui'

type Item = AppRouterOutputs['events']['list']
type Props = { item?: Item, filter?: string, loading?: boolean }
const props = withDefaults(defineProps<Props>(), {
  item: () => ({ type: 'FeatureCollection', features: [] }),
})

const emit = defineEmits<{ refresh: [] }>()

type Keys = keyof Item['features'][number]['properties']

const columns: TableColumn<Item>[] = [
  {
    id: 'boardId' as const,
    accessorKey: 'properties.boardId',
    header: 'Board ID',
  },
  {
    id: 'lat' as const,
    accessorKey: 'properties.lat',
    header: 'Latitude',
  },
  {
    id: 'lon' as const,
    accessorKey: 'properties.lon',
    header: 'Longitude',
  },
  {
    id: 'alt' as const,
    accessorKey: 'properties.alt',
    header: 'Altitude',
  },
  {
    id: 'speed' as const,
    accessorKey: 'properties.speed',
    header: 'Speed',
  },
  {
    id: 'heading' as const,
    accessorKey: 'properties.heading',
    header: 'Heading',
  },
  {
    id: 'hdop' as const,
    accessorKey: 'properties.hdop',
    header: 'HDOP',
  },
  {
    id: 'sat' as const,
    accessorKey: 'properties.sat',
    header: 'Satellites',
  },
  {
    id: 'ts' as const,
    accessorKey: 'properties.ts',
    header: 'Timestamp',
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
]

const MAP_ID_TO_LABEL: Record<Keys, string> = columns.reduce(
  (a, c) => ({ ...a, [c.id as Keys]: c.header }),
  {} as Record<Keys, string>,
)

const visible: Ref<Record<Keys, boolean>> = ref({
  id: false,
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
      <div class="text-sm text-gray-500">
        Last item @
        <NuxtTime
          v-if="props.item.features[0]?.properties.createdAt"
          :datetime="props.item.features[0]?.properties.createdAt"
          title
          relative
        />
        <span v-else>—</span>
      </div>

      <UButton
        icon="i-lucide-refresh-cw"
        color="neutral"
        variant="ghost"
        size="sm"
        class="ml-auto"
        @click="emit('refresh')"
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
        icon: 'i-lucide-map-pin-off',
        label: 'No events found',
      }"
      :ui="{
        tr: 'hover:bg-muted cursor-pointer',
      }"
      class="flex-1"
    >
      <template #boardId-cell="{ row }">
        <span class="font-mono text-sm text-primary font-medium">
          {{ row.original.properties.boardId ?? '-' }}
        </span>
      </template>

      <template #lat-cell="{ row }">
        <span class="font-mono text-sm">
          {{ row.original.properties.lat.toFixed(6) ?? '-' }}
        </span>
      </template>

      <template #lon-cell="{ row }">
        <span class="font-mono text-sm">
          {{ row.original.properties.lon.toFixed(6) ?? '-' }}
        </span>
      </template>

      <template #alt-cell="{ row }">
        <span class="font-mono text-sm">
          {{ row.original.properties.alt?.toFixed(1) ?? '-' }}
        </span>
      </template>

      <template #speed-cell="{ row }">
        <span class="font-mono text-sm">
          {{ row.original.properties.speed?.toFixed(1) ?? '-' }}
        </span>
      </template>

      <template #heading-cell="{ row }">
        <span class="font-mono text-sm">
          {{ row.original.properties.heading?.toFixed(0) ?? '-' }}
        </span>
      </template>

      <template #hdop-cell="{ row }">
        <span class="font-mono text-sm">
          {{ row.original.properties.hdop?.toFixed(2) ?? '-' }}
        </span>
      </template>

      <template #sat-cell="{ row }">
        <span class="font-mono text-sm font-medium">
          {{ row.original.properties.sat ?? '-' }}
        </span>
      </template>

      <template #ts-cell="{ row }">
        <span class="font-mono text-sm">
          {{ row.original.properties.ts ?? '-' }}
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
    </UTable>
  </div>
</template>
